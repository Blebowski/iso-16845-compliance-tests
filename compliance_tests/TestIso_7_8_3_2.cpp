/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 24.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.3.2
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e ≤ SJW(D) on bit position DATA.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          DATA field
 *          FDF = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             e ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  Then, the recessive to dominant edge before this dominant stuff bit
 *  shall be delayed by additional e TQ(D)’s of recessive value at the
 *  beginning of this stuff bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed stuff bit to
 *  recessive. This recessive part of Phase_seg2 start at e − 1 TQ(D)
 *  after sampling point.
 * 
 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The wrong value of stuff bit shall cause an error frame.
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

class TestIso_7_8_3_2 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Enable TX to RX feedback
            canAgentConfigureTxToRxFeedback(true);

            // CAN FD enabled only!
            if (canVersion == CAN_2_0_VERSION ||
                canVersion == CAN_FD_TOLERANT_VERSION)
            {
                testResult = false;
                return false;
            }

            for (int i = 0; i < dataBitTiming.sjw; i++)
            {
                // CAN FD frame with bit rate shift
                uint8_t dataByte = 0x7F; // 7th data bit is dominant stuff bit!
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
                goldenFrame = new Frame(frameFlags, 1, &dataByte);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing Data byte positive resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Lengthen bit before stuff bit by e in monitored frame!
                 *      (This covers DUT re-synchronisation).
                 *   3. Force first e TQ D of dominant stuff bit of driven
                 *      frame to recessive (this creates phase error of e and
                 *      shifts sample point by e).
                 *   4. Force last PH2 - e - 1 TQ of dominant stuff bit to
                 *      recessive on driven bit. Since Recessive value was set
                 *      to one before sample point (sample point shifted by e),
                 *      this shall be bit error!
                 *   5. Insert active error frame on monitor from next frame!
                 *      Insert passive by driver to send all recessive.
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *beforeStuff = monitorBitFrame->getBitOf(5, BIT_TYPE_DATA);
                beforeStuff->lengthenPhase(PH2_PHASE, i + 1);

                // 7-th bit should be stuff bit
                Bit *driverStuffBit = driverBitFrame->getBitOf(6, BIT_TYPE_DATA);
                int bitIndex = driverBitFrame->getBitIndex(driverStuffBit);
                for (int j = 0; j < i + 1; j++)
                    driverStuffBit->getTimeQuanta(j)->forceValue(RECESSIVE);

                //driverStuffBit->shortenPhase(PH2_PHASE, dataBitTiming.ph2 - i);

                for (int j = i; j < dataBitTiming.ph2; j++)
                    driverStuffBit->getTimeQuanta(PH2_PHASE, j)->forceValue(RECESSIVE);

                driverBitFrame->insertPassiveErrorFrame(bitIndex + 2);
                monitorBitFrame->insertActiveErrorFrame(bitIndex + 1);

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