/****************************************************************************** 
 * 
 * ISO16845 Compliance tests 
 * Copyright (C) 2021-present Ondrej Ille
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 * 
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
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
using namespace test_lib;

class TestIso_7_7_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            size_t num_elem_tests = nominal_bit_timing.GetBitLengthTimeQuanta() -
                                    nominal_bit_timing.ph2_ -
                                    nominal_bit_timing.sjw_ - 1;

            for (size_t i = 0; i < num_elem_tests; i++)
            {
                ElementaryTest test = ElementaryTest(i + 1);
                test.e_ = nominal_bit_timing.sjw_ + 1 + i;
                AddElemTest(TestVariant::Common, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // CAN 2.0 frame, Base identifier, randomize others
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, IdentifierType::Base);

            // Base ID full of 1s, 5th will be dominant stuff bit!
            int id = pow(2,11) - 1;
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, id);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Prolong TSEG2 of driven bit before the stuff bit on 5th bit of identifier
             *      (delay stuff bit) by e. Prolong TSEG1 of monitored stuff bit by SJW. This
             *      corresponds to resync. by SJW!
             *   2. Force whole TSEG2 and last time quanta of TSEG1 of the stuff bit to Recessive.
             *      This corresponds to shortening the bit by TSEG2 + 1.
             *   3. Insert Active Error frame to monitored frame from next bit. Since monitored
             *      stuff bit was prolonged by SJW, this corresponds to expected positive
             *      resynchronisation and thus error frame will be monitored at exact expected
             *      position. Insert passive error frame to driven bit so that it will transmitt
             *      all recessive!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            Bit *before_stuff_bit = driver_bit_frm->GetBitOf(3, BitType::BaseIdentifier);
            before_stuff_bit->LengthenPhase(BitPhase::Ph2, elem_test.e_);

            // Monitor bit as if node re-synchronised by SJW!
            Bit *monitor_stuff_bit = monitor_bit_frm->GetStuffBit(0);
            monitor_stuff_bit->LengthenPhase(BitPhase::Sync, nominal_bit_timing.sjw_);

            Bit *driver_stuff_bit = driver_bit_frm->GetStuffBit(0);
            for (size_t j = 0; j < nominal_bit_timing.ph2_; j++)
                driver_stuff_bit->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);
            BitPhase prev_phase = driver_stuff_bit->PrevBitPhase(BitPhase::Ph2);
            int to_be_shortened = elem_test.e_ - nominal_bit_timing.sjw_ + 1;

            int shortened = driver_stuff_bit->ShortenPhase(prev_phase, to_be_shortened);

            if (shortened < to_be_shortened)
            {
                prev_phase = driver_stuff_bit->PrevBitPhase(prev_phase);
                driver_stuff_bit->ShortenPhase(prev_phase, to_be_shortened - shortened);
            }

            int index = driver_bit_frm->GetBitIndex(driver_stuff_bit);
            monitor_bit_frm->InsertActiveErrorFrame(index + 1);
            driver_bit_frm->InsertPassiveErrorFrame(index + 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            // Clean REC so that errors don't accumulate over testing!
            TestMessage("Testing positive phase error: %d", elem_test.e_);
            dut_ifc->SetRec(0);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};