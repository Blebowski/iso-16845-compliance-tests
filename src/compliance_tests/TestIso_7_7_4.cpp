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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            size_t num_elem_tests = nbt.GetBitLenTQ() -
                                    nbt.ph2_ -
                                    nbt.sjw_ -
                                    1;

            assert(num_elem_tests > 0 && "Number of elementary tests positive and non-zero!");

            for (size_t i = 0; i < num_elem_tests; i++)
            {
                ElemTest test = ElemTest(i + 1);
                test.e_ = static_cast<int>(nbt.sjw_ + i + 1);
                AddElemTest(TestVariant::Common, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // CAN 2.0 frame, Base identifier, randomize others
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, IdentKind::Base);

            // Base ID full of 1s, 5th will be dominant stuff bit!
            int id = CAN_BASE_ID_ALL_ONES;
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

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
            mon_bit_frm->ConvRXFrame();
            Bit *before_stuff_bit = drv_bit_frm->GetBitOf(3, BitKind::BaseIdent);
            before_stuff_bit->LengthenPhase(BitPhase::Ph2, elem_test.e_);

            // Monitor bit as if node re-synchronised by SJW!
            Bit *monitor_stuff_bit = mon_bit_frm->GetStuffBit(0);
            monitor_stuff_bit->LengthenPhase(BitPhase::Sync, nbt.sjw_);

            Bit *driver_stuff_bit = drv_bit_frm->GetStuffBit(0);
            for (size_t j = 0; j < nbt.ph2_; j++)
                driver_stuff_bit->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);
            BitPhase prev_phase = driver_stuff_bit->PrevBitPhase(BitPhase::Ph2);

            size_t to_be_shortened = static_cast<size_t>(elem_test.e_) + 1 - nbt.sjw_;
            assert(to_be_shortened < 100000 && "'to_be_shortened' underflow");

            size_t shortened = driver_stuff_bit->ShortenPhase(prev_phase, to_be_shortened);

            if (shortened < to_be_shortened)
            {
                prev_phase = driver_stuff_bit->PrevBitPhase(prev_phase);
                driver_stuff_bit->ShortenPhase(prev_phase, to_be_shortened - shortened);
            }

            size_t index = drv_bit_frm->GetBitIndex(driver_stuff_bit);
            mon_bit_frm->InsertActErrFrm(index + 1);
            drv_bit_frm->InsertPasErrFrm(index + 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            // Clean REC so that errors don't accumulate over testing!
            TestMessage("Testing positive phase error: %d", elem_test.e_);
            dut_ifc->SetRec(0);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            return FinishElemTest();
        }
};