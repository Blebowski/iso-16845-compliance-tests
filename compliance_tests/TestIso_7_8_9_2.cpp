/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.9.2
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point
 *        is the same as the bus value immediately after the edge on bit
 *        position DATA.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          DATA field
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the stuff bit to dominant from the second TQ(D)
 *             until the beginning of Phase_Seg2(D).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a recessive stuff bit in data phase
 *  The LT forces a recessive stuff bit inside DATA field to dominant
 *  according to elementary test cases.
 *
 * Response:
 *  The modified stuff bit shall be sampled as dominant.
 *  The dominant sampled stuff bit shall be detected as a stuff error and
 *  shall be followed by an error frame..
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

class TestIso_7_8_9_2 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift.
            FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);

            // Recessive stuff bit on 7-th data bit!
            uint8_t dataByte = 0x80;
            goldenFrame = new Frame(frameFlags, 0x1, &dataByte);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("No synchronisation after dominant bit sampled on Data field bit!");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force 7-th bit of data field to Dominant from 2nd time quanta
             *      till beginning of PH2.
             *   3. Insert expected active error frame on monitored frame from
             *      8-th bit of data field. Insert passive error frame on driven
             *      frame!
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *stuffBit = driverBitFrame->getBitOf(6, BIT_TYPE_DATA);
            stuffBit->forceTimeQuanta(1, dataBitTiming.prop + dataBitTiming.ph1, DOMINANT);

            driverBitFrame->insertActiveErrorFrame(driverBitFrame->getBitOf(7, BIT_TYPE_DATA));
            monitorBitFrame->insertActiveErrorFrame(monitorBitFrame->getBitOf(7, BIT_TYPE_DATA));

            driverBitFrame->print(true);
            monitorBitFrame->print(true);

            // Push frames to Lower tester, run and check!
            pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
            runLowerTester(true, true);
            checkLowerTesterResult();

            deleteCommonObjects();

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};