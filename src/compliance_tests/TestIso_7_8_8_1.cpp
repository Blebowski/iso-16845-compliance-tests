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
 * @date 20.6.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.8.1
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there is an additional recessive
 *        to dominant edge between two sample points where the first edge comes
 *        before the synchronization segment on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Bit start with negative offset and glitch between synchronization
 *      segment and sample point.
 *          ESI = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT reduce the length of BRS bit by one TQ(D) and the LT force
 *             the second TQ of ESI to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  Additionally, the Phase_Seg2(D) of ESI bit shall be forced to recessive.
 *
 * Response:
 *  The modified ESI bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_8_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            AddElemTest(TestVariant::CanFdEna, ElemTest(1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift,
                                                       EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten BRS by 1 TQ in driven and monitored frame.
             *   3. Force 2nd TQ of ESI to Recessive.
             *   4. Force Phase 2 of ESI to Recessive.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *brs_bit = drv_bit_frm->GetBitOf(0, BitKind::Brs);
            Bit *brs_bit_monitor = mon_bit_frm->GetBitOf(0, BitKind::Brs);
            Bit *esi_bit = drv_bit_frm->GetBitOf(0, BitKind::Esi);

            brs_bit->ShortenPhase(BitPhase::Ph2, 1);
            brs_bit_monitor->ShortenPhase(BitPhase::Ph2, 1);

            esi_bit->ForceTQ(1, BitVal::Recessive);
            esi_bit->ForceTQ(0, dbt.ph2_ - 1, BitPhase::Ph2, BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Glitch filtering test for negative phase error on ESI bit");
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};