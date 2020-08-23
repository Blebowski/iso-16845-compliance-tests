/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.4
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e > SJW(N).
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
 *          e ∈ [SJW(N) + 1], [NTQ(N) − Phase_Seg2(N) − 1].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT delays a dominant stuff bit in arbitration field by an amount of
 *  e time quanta and shortens the same bit by an amount of [Phase_Seg2(N) 
 *  + 1TQ + e − SJW(N)] according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame 1 bit time – [e − SJW(N)] time quanta
 *  after the recessive to dominant edge of the delayed stuff bit.
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

class TestIso_7_7_4 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Enable TX to RX feedback
            CanAgentConfigureTxToRxFeedback(true);

            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/

            for (int e = nominal_bit_timing.sjw_ + 1;
                 e <= nominal_bit_timing.prop_ + nominal_bit_timing.ph1_;
                 e++)
            {
                // Clean REC so that errors don't accumulate over testing!
                dut_ifc->SetRec(0);

                // CAN 2.0 frame, Base identifier, randomize others
                FrameFlags frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base);

                // Base ID full of 1s, 5th will be dominant stuff bit!
                int id = pow(2,11) - 1;
                golden_frame = new Frame(frameFlags, 0x1, id);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing positive phase error: %d", e);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Prolong TSEG2 of driven bit before the stuff bit on 5th
                 *      bit of identifier (delay stuff bit) by e. Prolong TSEG1
                 *      of monitored stuff bit by SJW. This corresponds to resync.
                 *      by SJW!
                 *   2. Force whole TSEG2 and last time quanta of TSEG1 to
                 *      Recessive. This corresponds to shortening the bit by
                 *      TSEG2 + 1.
                 *   3. Insert Active Error frame to monitored frame from next
                 *      bit. Since monitored stuff bit was prolonged by SJW, this
                 *      corresponds to expected positive resynchronisation and
                 *      thus error frame will be monitored at exact expected
                 *      position. Insert passive error frame to driven bit so that
                 *      it will transmitt all recessive!
                 */
                monitor_bit_frame->TurnReceivedFrame();
                Bit *beforeStuffBit = driver_bit_frame->GetBitOf(3, BitType::BaseIdentifier);
                beforeStuffBit->LengthenPhase(BitPhase::Ph2, e);

                // Monitor bit as if node re-synchronised by SJW!
                Bit *monitorStuffBit = monitor_bit_frame->GetStuffBit(0);
                monitorStuffBit->LengthenPhase(BitPhase::Sync, nominal_bit_timing.sjw_);

                Bit *driverStuffBit = driver_bit_frame->GetStuffBit(0);
                for (int j = 0; j < nominal_bit_timing.ph2_; j++)
                    driverStuffBit->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);

                BitPhase prevPhase = driverStuffBit->PrevBitPhase(BitPhase::Ph2);
                int toBeShortened = e - nominal_bit_timing.sjw_ + 1;
                
                int shortened = driverStuffBit->ShortenPhase(prevPhase, toBeShortened);

                if (shortened < toBeShortened)
                {
                    prevPhase = driverStuffBit->PrevBitPhase(prevPhase);
                    driverStuffBit->ShortenPhase(prevPhase, toBeShortened - shortened);
                }

                int index = driver_bit_frame->GetBitIndex(driverStuffBit);
                monitor_bit_frame->InsertActiveErrorFrame(index + 1);
                driver_bit_frame->InsertPassiveErrorFrame(index + 1);
                
                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

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