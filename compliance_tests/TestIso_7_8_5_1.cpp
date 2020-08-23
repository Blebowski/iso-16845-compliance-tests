/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 13.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.1
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW(D) on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          ESI = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *              |e| ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant ESI bit.
 *  The LT shortened the BRS bit by an amount of |e| TQ according to ele-
 *  mentary test cases.
 *  Additionally, the ESI bit shall be forced to recessive value from
 *  [Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) − e] up to end of bit.
 *
 * Response:
 *  The modified ESI bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
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

class TestIso_7_8_5_1 : public test_lib::TestBase
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

            int upperTh = data_bit_timing.ph1_ + data_bit_timing.prop_ + 1;

            for (int i = 1; i <= data_bit_timing.sjw_; i++)
            {
                // CAN FD frame with bit rate shift, ESI = Dominant
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift,
                                                    EsiFlag::ErrorActive);
                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing ESI negative resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Shorten PH2 of BRS by e.
                 *   3. Force ESI to Recessive from Sync+Prop+Phase1-e till the
                 *      end of bit.
                 */
                monitor_bit_frame->TurnReceivedFrame();

                Bit *brsBit = driver_bit_frame->GetBitOf(0, BitType::Brs);
                Bit *esiBit = driver_bit_frame->GetBitOf(0, BitType::Esi);
                Bit *brsBitMonitor = monitor_bit_frame->GetBitOf(0, BitType::Brs);


                /**************************************************************
                 * These modifications are how I think this was meant!!
                 **************************************************************/
                brsBit->ShortenPhase(BitPhase::Ph2, i);
                brsBitMonitor->ShortenPhase(BitPhase::Ph2, i);

                for (int j = 0; j < data_bit_timing.ph2_; j++)
                    esiBit->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);


                /**************************************************************
                 * This is exactly how TC describes it in ISO16845-1 2016 and I
                 * think it is wrong!!!
                 **************************************************************/
                // Shorten BRS by e
                // brsBit->shortenPhase(PH2_PHASE, i);

                //int startTq = 1 + dataBitTiming.prop + dataBitTiming.ph1 - i;
                //for (int j = startTq; j < brsBit->getLenTimeQuanta(); j++)
                //    esiBit->ForceTimeQuanta(j, RECESSIVE);


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