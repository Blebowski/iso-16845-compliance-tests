/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 10.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.6
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a negative phase error e on a recessive to dominant edge
 *        with |e| > SJW(N).
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *      
 *      #1 The values tested for e are measured in time quanta with
 *          |e| ∈ {[(SJW(N) + 1], Phase_Seg2(N)}.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT shortens the last recessive bit before an expected dominant stuff
 *  bit in arbitration field by an amount of |e| time quanta and then sends
 *  a dominant value for one time quantum followed by a recessive state
 *  according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame 1 bit time + [|e| − SJW(N)] time
 *  quanta after the last recessive to dominant edge.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "../vpi_lib/vpiComplianceLib.hpp"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"
#include "../test_lib/TestLoader.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;

class TestIso_7_7_6 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Enable TX to RX feedback
            canAgentConfigureTxToRxFeedback(true);

            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/

            for (int i = nominalBitTiming.sjw; i < nominalBitTiming.ph2; i++)
            {
                // CAN 2.0 frame, Base identifier, randomize others
                FrameFlags frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER);

                // Base ID full of 1s, 5th will be dominant stuff bit!
                int id = pow(2,11) - 1;
                goldenFrame = new Frame(frameFlags, 0x1, id);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing negative phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Shorten TSEG2 of bit before first stuff bit by e
                 *      in driven frame. In monitored frame, shorten only
                 *      by SJW as this corresponds to how DUT has
                 *      resynchronised!
                 *   2. Set bit value of Dominant stuff bit to Recessive
                 *      apart from 1 TQ in the beginning of the bit for
                 *      driven frame!
                 *   3. Insert expected error frame one bit after first
                 *      stuff bit! Since bit before stuff bit was shortened
                 *      by SJW, start of error frame in monitored frame
                 *      should be at exact position as DUT should transmit
                 *      it! Insert Passive Error frame to driver so that
                 *      it sends all recessive values!
                 */
                monitorBitFrame->turnReceivedFrame();

                driverBitFrame->getBitOf(4, BIT_TYPE_BASE_ID)->shortenPhase(
                    PH2_PHASE, i + 1);
                monitorBitFrame->getBitOf(4, BIT_TYPE_BASE_ID)->shortenPhase(
                    PH2_PHASE, nominalBitTiming.sjw);

                Bit *stuffBit = driverBitFrame->getStuffBit(0);
                stuffBit->setBitValue(RECESSIVE);
                stuffBit->getTimeQuanta(0)->forceValue(DOMINANT);

                int index = driverBitFrame->getBitIndex(stuffBit);
                monitorBitFrame->insertActiveErrorFrame(index + 1);
                driverBitFrame->insertPassiveErrorFrame(index + 1);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                deleteCommonObjects();
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};