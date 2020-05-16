/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 3.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.9.1
 * 
 * @brief The purpose of this test is to verify that an IUT will not detect an
 *        SOF when detected dominant level ≤ [Prop_Seg(N) + Phase_Seg1(N) −
 *        1 TQ(N)].
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *  Glitch Pulse length = Prop_Seg(N) + Phase_Seg1(N) − 1 TQ(N)
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *      #1 Dominant pulse on IDLE bus [Prop_Seg(N) + Phase_Seg1(N) − 1 TQ(N)].
 *      Refer to 6.2.3.     
 * 
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a dominant glitch according to elementary test cases for
 *  this test case. Then the LT waits for 8 bit times.
 * 
 * Response:
 *  The IUT shall remain in the idle state.
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

class TestIso_7_7_9_1 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/

            // CAN 2.0 frame, randomize others
            FrameFlags frameFlags = FrameFlags(CAN_2_0);
            goldenFrame = new Frame(frameFlags);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Glitch filtering in idle state - single glitch");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Remove all bits but first from monitored frame.
             *   2. Remove all bits but first from driven frame.
             *   3. Shorten SOF to PROP + PH1 - 1 Time quanta in driven frame.
             *   4. Insert 9 recessive bits to monitor.
             */
            driverBitFrame->clearBitsFrom(1);
            monitorBitFrame->clearBitsFrom(1);
            monitorBitFrame->getBit(0)->setBitValue(RECESSIVE);

            driverBitFrame->getBit(0)->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);
            driverBitFrame->getBit(0)->shortenPhase(SYNC_PHASE, 1);
            BitPhase phase = driverBitFrame->getBit(0)->prevBitPhase(PH2_PHASE);
            driverBitFrame->getBit(0)->shortenPhase(phase, 1);
            
            for (int i = 0; i < 9; i++)
            {
                monitorBitFrame->insertBit(Bit(BIT_TYPE_SOF, RECESSIVE,
                    &frameFlags, &nominalBitTiming, &dataBitTiming), 1);
                driverBitFrame->insertBit(Bit(BIT_TYPE_SOF, RECESSIVE,
                    &frameFlags, &nominalBitTiming, &dataBitTiming), 1);
            }

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