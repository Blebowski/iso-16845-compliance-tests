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
 * @test ISO16845 7.8.3.1
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e ≤ SJW(D) on bit position ESI.
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
 *          #1 The values tested for e are measured in time quanta with
 *             e ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The first e TQ(D) (e according to elementary test cases) of the ESI bit
 *  are set to recessive, then the following {Prop_Seg(D) + Phase_Seg1(D)]
 *  TQ(D)’s are set to dominant. The rest of the ESI bit, Phase_Seg2(D)+1 is
 *  set to recessive. At all, ESI is lengthened by e TQ(D).
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

class TestIso_7_8_3_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = 1; i <= dbt.sjw_; i++)
            {
                ElemTest test = ElemTest(i);
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
             *   2. Lengthen SYNC phase of ESI by e (both driven and monitored frame).
             *   3. Force Prop + PH1 TQ after initial e TQ to dominant!
             **************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *esi_bit_driver = drv_bit_frm->GetBitOf(0, BitKind::Esi);
            Bit *esi_bit_monitor = mon_bit_frm->GetBitOf(0, BitKind::Esi);

            esi_bit_driver->LengthenPhase(BitPhase::Sync, elem_test.e_);
            esi_bit_monitor->LengthenPhase(BitPhase::Sync, elem_test.e_);

            for (size_t j = 0; j < dbt.prop_ + dbt.ph1_; j++)
                esi_bit_driver->ForceTQ(elem_test.e_ + j, BitVal::Dominant);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ESI positive resynchronisation with phase error: %d", elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};