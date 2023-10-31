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
 * @test ISO16845 7.8.2.2
 *
 * @brief The purpose of this test is to verify that the IUT makes a hard
 *        synchronization when receiving an early recessive to dominant edge
 *        between FDF and “res” bit by e, where:
 *            e = Phase_Seg2(N)
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) configuration as available by IUT.
 *          SJW(N) = 1
 *          res
 *          FDF = 1
 *          BRS = 0
 *          ESI = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The LT generates a valid frame with shortened FDF bit by an amount
 *             of e = Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  The LT sets the last Phase_Seg2(D) TQ of the dominant BRS bit to recessive.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The hard synchronization shall correct the maximum phase error as defined
 *  in ISO 11898-1.
 *  The frame is valid. No error flag shall occur. The bit rate will not switch
 *  for data phase.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_2_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            ElemTest test = ElemTest(1);
            test.e_ = nbt.ph2_;
            AddElemTest(TestVariant::CanFdEna, std::move(test));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::NoShift);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten PH2 of FDF/EDL bit to 0 (both driven and monitored frames since DUT
             *      shall Hard synchronize)
             *   3. Force TSEG2 of BRS to Recessive on driven frame!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *edl_bit_driver = drv_bit_frm->GetBitOf(0, BitKind::Edl);
            Bit *edl_bit_monitor = mon_bit_frm->GetBitOf(0, BitKind::Edl);
            Bit *brs_bit = drv_bit_frm->GetBitOf(0, BitKind::Brs);

            edl_bit_driver->ShortenPhase(BitPhase::Ph2, nbt.ph2_);
            edl_bit_monitor->ShortenPhase(BitPhase::Ph2, nbt.ph2_);

            for (size_t j = 0; j < dbt.ph2_; j++)
                brs_bit->GetTQ(BitPhase::Ph2, j)->ForceVal(BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing 'res' bit hard-sync with negative phase error");
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};