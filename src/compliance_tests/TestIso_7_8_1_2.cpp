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
 * @test ISO16845 7.8.1.2
 *
 * @brief The purpose of this test is to verify the position of the sample
 *        point of an IUT on bit position DATA field.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *      DATA field
 *      FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform for at least 1 bit rate
 *      configuration:
 *          #1 test for early sampling point: bit level change to recessive
 *             before sampling point;
 *          #2 test for late sampling point: bit level change to recessive
 *             after sampling point.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  Test Data #1:
 *      The LT forces a recessive bit to dominant from beginning up to one TQ(D)
 *      before the sampling point.
 *
 *  Test DATA #2:
 *      The LT forces a dominant bit to recessive for Phase_Seg2(D).
 *
 * Response:
 *  Test DATA #1:
 *      The modified data bit shall be sampled as recessive.
 *      The frame is valid. DontShift error flag shall occur.
 *
 *  Test DATA #2:
 *      The modified data bit shall be sampled as dominant.
 *      The frame is valid. DontShift error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_1_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = 0; i < 2; i++) {
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::Can20));
            }
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // To avoid stuff bits in data field.
            uint8_t data_byte_recessive_sampled = 0x55;
            uint8_t data_byte_dominant_sampled = 0x15;

            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift);

            // In 2nd iteration dominant bit will be sampled at second bit position of data field!
            // We must expect this in golden frame so that it will be compared correctly with
            // received frame!
            if (elem_test.index_ == 1)
                gold_frm = std::make_unique<Frame>(*frm_flags, 1, &data_byte_recessive_sampled);
            else
                gold_frm = std::make_unique<Frame>(*frm_flags, 1, &data_byte_dominant_sampled);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received.
             *   2. Modify 2nd bit of data field. Since data is 0x55 this bit is recessive. Flip
             *      its TSEG - 1 (elem test 1) or TSEG1 (elem test 2) to dominant.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *data_bit = drv_bit_frm->GetBitOf(1, BitKind::Data);
            data_bit->val_ = BitVal::Recessive;

            size_t dominant_pulse_length;
            if (elem_test.index_ == 1)
                dominant_pulse_length = dbt.prop_ + dbt.ph1_;
            else
                dominant_pulse_length = dbt.prop_ + dbt.ph1_ + 1;

            for (size_t j = 0; j < dominant_pulse_length; j++)
                data_bit->ForceTQ(j, BitVal::Dominant);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            if (elem_test.index_ == 1)
                TestMessage("Testing Data bit sampled Recessive");
            else
                TestMessage("Testing Data bit sampled Dominant");

            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};