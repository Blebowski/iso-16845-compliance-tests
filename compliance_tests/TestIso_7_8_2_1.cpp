/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 23.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.2.1
 * 
 * @brief The purpose of this test is to verify that the IUT makes a hard
 *        synchronization when receiving a recessive to dominant edge delayed
 *        by e, where:
 *          e ∈ [SJW(N) + 1, NTQ(N) − Phase_Seg2(N) − 1]
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *          “res” bit
 *          FDF = 1
 *          BRS = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The LT generates a valid frame with prolonged FDF bit by an
 *             amount of e ∈ [SJW(N) + 1, NTQ(N) − Phase_Seg2(N) − 1].
 *      
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  The LT sets the first [Prop_Seg(N) + Phase_Seg1(N)] TQ’s of the recessive
 *  BRS bit to dominant.
 * 
 * Response:
 *  The modified BRS bit shall be sampled as recessive.
 *  The hard synchronization shall correct the maximum phase error as defined
 *  in ISO 11898-1.
 *  The frame is valid. No error flag shall occur.
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

class TestIso_7_8_2_1 : public test_lib::TestBase
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

            /*****************************************************************
             * CRC Delimiter sampled Recessive (OK) /
             * CRC Delimiter sampled Dominant (Error frame)
             ****************************************************************/
            int highTh = nominalBitTiming.prop + nominalBitTiming.ph1 + 1;
            for (int i = nominalBitTiming.sjw + 1; i < highTh; i++)
            {
                // CAN FD frame
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing 'res' bit hard-sync with phase error: %d", i);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Prolong FDF/EDL bit by e (both driven and monitored
                 *      frame since DUT shall execute hard sync).
                 *   3. Set first Prop+Ph1 TQ of BRS to Dominant.
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *edlBitDriver = driverBitFrame->getBitOf(0, BIT_TYPE_EDL);
                Bit *edlBitMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_EDL);
                Bit *brsBit = driverBitFrame->getBitOf(0, BIT_TYPE_BRS);

                edlBitDriver->lengthenPhase(PH2_PHASE, i);
                edlBitMonitor->lengthenPhase(PH2_PHASE, i);

                for (int j = 0; j < (nominalBitTiming.ph1 + nominalBitTiming.prop); j++)
                    brsBit->getTimeQuanta(j)->forceValue(DOMINANT);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                // Read received frame from DUT and compare with sent frame
                Frame readFrame = this->dutIfc->readFrame();
                if (compareFrames(*goldenFrame, readFrame) == false)
                {
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                }

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