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

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/

            // CAN 2.0 frame, randomize others
            FrameFlags frameFlags = FrameFlags(FrameType::Can2_0);
            golden_frame = new Frame(frameFlags);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("Glitch filtering in idle state - single glitch");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Remove all bits but first from monitored frame.
             *   2. Remove all bits but first from driven frame.
             *   3. Shorten SOF to PROP + PH1 - 1 Time quanta in driven frame.
             *   4. Insert 9 recessive bits to monitor.
             */
            driver_bit_frame->RemoveBitsFrom(1);
            monitor_bit_frame->RemoveBitsFrom(1);
            monitor_bit_frame->GetBit(0)->bit_value_ = BitValue::Recessive;

            driver_bit_frame->GetBit(0)->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            driver_bit_frame->GetBit(0)->ShortenPhase(BitPhase::Sync, 1);
            BitPhase phase = driver_bit_frame->GetBit(0)->PrevBitPhase(BitPhase::Ph2);
            driver_bit_frame->GetBit(0)->ShortenPhase(phase, 1);
            
            for (int i = 0; i < 9; i++)
            {
                monitor_bit_frame->InsertBit(BitType::Sof, BitValue::Recessive, 1);
                driver_bit_frame->InsertBit(BitType::Sof, BitValue::Recessive, 1);
            }

            driver_bit_frame->Print(true);
            monitor_bit_frame->Print(true);

            // Push frames to Lower tester, run and check!
            PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            DeleteCommonObjects();

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};