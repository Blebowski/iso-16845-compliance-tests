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
 * @date 23.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.4.1
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e > SJW(D) on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          ESI = 1
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              e ∈ {[SJW(D) + 1], [NTQ(D) − Phase_Seg2(D) − 1]{
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with recessive ESI bit.
 *  The LT invert the value of ESI bit to dominant value.
 *  Then, the recessive to dominant edge between BRS and ESI shall be delayed
 *  by additional e TQ(D)’s of recessive value at the beginning of ESI bit
 *  according to elementary test cases.
 *
 *  The LT forces a part of Phase_Seg2(D) of the delayed ESI bit to recessive.
 *  This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after sampling
 *  point.
 *
 * Response:
 *  The modified ESI bit shall be sampled as recessive.
 *  The frame is valid. DontShift error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_4_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = dbt.sjw_ + 1;
                 i <= dbt.GetBitLenTQ() - dbt.ph2_ - 1;
                 i++)
            {
                ElemTest test = ElemTest(i - dbt.sjw_);
                test.e_ = i;
                AddElemTest(TestVariant::CanFdEna, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);

            assert(nbt.brp_ == dbt.brp_ &&
                   "TQ(N) shall equal TQ(D) for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift,
                                                       EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force ESI value to dominant.
             *   3. Force first e time quantas of ESI bit to Recessive
             *   4. Force ESI from SJW - 1 after sample point till the end to Recessive.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *esi_bit = drv_bit_frm->GetBitOf(0, BitKind::Esi);
            esi_bit->val_ = BitVal::Dominant;

            for (int j = 0; j < elem_test.e_; j++)
                esi_bit->ForceTQ(j, BitVal::Recessive);

            for (size_t j = dbt.sjw_ - 1; j < dbt.ph2_; j++)
                esi_bit->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ESI positive resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};