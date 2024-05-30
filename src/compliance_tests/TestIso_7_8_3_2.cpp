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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_3_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = 1; i <= dbt.sjw_; i++)
            {
                ElemTest test = ElemTest(i);
                test.e_ = static_cast<int>(i);
                AddElemTest(TestVariant::CanFdEna, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x7F; // 7th data bit is dominant stuff bit!
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift);
            gold_frm = std::make_unique<Frame>(*frm_flags, 1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Lengthen bit before stuff bit by e in monitored frame! (This covers DUT
             *      re-synchronisation).
             *   3. Force first e TQ D of dominant stuff bit of driven frame to recessive
             *      (this creates phase error of e and shifts sample point by e).
             *   4. Force last PH2 - e - 1 TQ of dominant stuff bit to recessive on driven bit.
             *      Since Recessive value was set to one before sample point (sample point shifted
             *      by e), this shall be bit error!
             *   5. Insert active error frame on monitor from next bit, Insert passive by driver
             *      to send all recessive.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *before_stuff_bit = mon_bit_frm->GetBitOf(5, BitKind::Data);
            before_stuff_bit->LengthenPhase(BitPhase::Ph2, static_cast<size_t>(elem_test.e_));

            // 7-th bit should be stuff bit
            Bit *driver_stuff_bit = drv_bit_frm->GetBitOf(6, BitKind::Data);
            TEST_ASSERT(driver_stuff_bit->val_ == BitVal::Dominant, "Driven Stuff Bit Dominant");

            size_t bit_index = drv_bit_frm->GetBitIndex(driver_stuff_bit);
            for (size_t j = 0; j < static_cast<size_t>(elem_test.e_); j++)
                driver_stuff_bit->GetTQ(j)->ForceVal(BitVal::Recessive);

            //driver_stuff_bit->shortenPhase(PH2_PHASE, dataBitTiming.ph2 - i);

            TEST_ASSERT(elem_test.e_ > 0, "'j' will underflow!");
            for (size_t j = elem_test.e_ - 1; j < dbt.ph2_; j++)
                driver_stuff_bit->GetTQ(BitPhase::Ph2, j)
                    ->ForceVal(BitVal::Recessive);

            drv_bit_frm->InsertPasErrFrm(bit_index + 2);
            mon_bit_frm->InsertActErrFrm(bit_index + 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing Data byte positive resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            return FinishElemTest();
        }
};