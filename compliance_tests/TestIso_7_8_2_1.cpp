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
 *  The frame is valid. DontShift error flag shall occur.
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

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Enable TX to RX feedback
            CanAgentConfigureTxToRxFeedback(true);

            // CAN FD enabled only!
            if (dut_can_version == CanVersion::Can_2_0 ||
                dut_can_version == CanVersion::CanFdTolerant)
            {
                test_result = false;
                return false;
            }

            /*****************************************************************
             * CRC Delimiter sampled Recessive (OK) /
             * CRC Delimiter sampled Dominant (Error frame)
             ****************************************************************/
            int highTh = nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ + 1;
            for (int i = nominal_bit_timing.sjw_ + 1; i < highTh; i++)
            {
                // CAN FD frame
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift);
                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing 'res' bit hard-sync with phase error: %d", i);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Prolong FDF/EDL bit by e (both driven and monitored
                 *      frame since DUT shall execute hard sync).
                 *   3. Set first Prop+Phase1 TQ of BRS to Dominant.
                 */
                monitor_bit_frame->TurnReceivedFrame();

                Bit *edlBitDriver = driver_bit_frame->GetBitOf(0, BitType::Edl);
                Bit *edlBitMonitor = monitor_bit_frame->GetBitOf(0, BitType::Edl);
                Bit *brsBit = driver_bit_frame->GetBitOf(0, BitType::Brs);

                edlBitDriver->LengthenPhase(BitPhase::Ph2, i);
                edlBitMonitor->LengthenPhase(BitPhase::Ph2, i);

                for (size_t j = 0; j < (nominal_bit_timing.ph1_ + nominal_bit_timing.prop_); j++)
                    brsBit->GetTimeQuanta(j)->ForceValue(BitValue::Dominant);

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                // Read received frame from DUT and compare with sent frame
                Frame readFrame = this->dut_ifc->ReadFrame();
                if (CompareFrames(*golden_frame, readFrame) == false)
                {
                    test_result = false;
                    TestControllerAgentEndTest(test_result);
                }

                DeleteCommonObjects();
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};