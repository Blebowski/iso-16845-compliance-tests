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
 * @date 16.6.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.2
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW(D) on bit position DATA.
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
 *              |e| ∈ [1, SJW(D)]
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  The LT shortened the DATA bit before the dominant stuff bit by an amount
 *  of |e| TQ according to elementary test cases.
 *  Additionally, the Phase_Seg2(D) of the dominant stuff bit shall be forced
 *  to recessive.
 *
 * Response:
 *  The modified stuff bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_5_2 : public test::TestBase
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
             *   2. Shorten 6-th bit of data field by e TQ. This should be a bit before stuff bit.
             *      Shorten both in driver and monitor.
             *   3. Force PH2 of 7-th bit of data field to Recessive. This should be a stuff bit.
             *      Do it on driven frame only.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *driver_before_stuff_bit = drv_bit_frm->GetBitOf(5, BitKind::Data);
            Bit *monitor_before_stuff_bit = mon_bit_frm->GetBitOf(5, BitKind::Data);
            Bit *driver_stuff_bit = drv_bit_frm->GetBitOf(6, BitKind::Data);

            driver_before_stuff_bit->ShortenPhase(BitPhase::Ph2, elem_test.e_);
            monitor_before_stuff_bit->ShortenPhase(BitPhase::Ph2,  elem_test.e_);

            for (size_t j = 0; j < dbt.ph2_; j++)
                driver_stuff_bit->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing data byte negative resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};