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
 * @test ISO16845 7.8.4.2
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e > SJW(D) on bit position DATA.
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
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  Then, the recessive to dominant edge before this dominant stuff bit shall
 *  be delayed by additional e TQ(D)’s of recessive value at the beginning of
 *  this stuff bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed stuff bit to rece-
 *  ssive. This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after
 *  sampling point.
 *
 *  The LT forces a part of Phase_Seg2(D) of the delayed ESI bit to recessive.
 *  This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after sampling
 *  point.
 *
 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The wrong value of stuff bit shall cause an error flag.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_4_2 : public test::TestBase
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
                test.e_ = static_cast<int>(i);
                AddElemTest(TestVariant::CanFdEna, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x7F;
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force first e time quantas of 7-th data bit to Recessive. This bit should be
             *      dominant stuff bit.
             *   3. Force 7-th data bit from SJW - 1 after sample point till the end to Recessive.
             *   4. Lengthen monitored 7-th data bit by SJW (this correspond to DUTs resync. by SJW).
             *   5. Insert active error frame from 8-th data bit further to monitored frame. Insert
             *      passive error frame to driven frame!
             **************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *driver_stuff_bit = drv_bit_frm->GetBitOf(6, BitKind::Data);
            Bit *monitor_stuff_bit = mon_bit_frm->GetBitOf(6, BitKind::Data);

            // One bit after stuff bit will be recessive due to data byte. Insert
            // passive error frame from one bit further so that model does not modify
            // the stuff bit due to insertion of error frame after bit in data bit rate!
            Bit *driver_next_bit = drv_bit_frm->GetBitOf(8, BitKind::Data);
            Bit *monitor_next_bit = mon_bit_frm->GetBitOf(7, BitKind::Data);

            for (int j = 0; j < elem_test.e_; j++)
                driver_stuff_bit->ForceTQ(j, BitVal::Recessive);
            for (size_t j = dbt.sjw_ - 1; j < dbt.ph2_; j++)
                driver_stuff_bit->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);

            monitor_stuff_bit->LengthenPhase(BitPhase::Sync, dbt.sjw_);

            drv_bit_frm->InsertPasErrFrm(driver_next_bit);
            mon_bit_frm->InsertActErrFrm(monitor_next_bit);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing data byte positive resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            return FinishElemTest();
        }
};