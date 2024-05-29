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
 * @date 4.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.9
 *
 * @brief This test verifies the behaviour of the IUT when receiving two
 *        consecutive frames not separated by a bus idle state.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Intermission field length
 *      FDF = 0
 *
 *  CAN FD Enabled
 *      Intermission field length
 *      FDF = 1
 *
 * Elementary test cases:
 *      #1 The second frame starts after the second intermission bit of the
 *         first frame.
 *      #2 The second frame starts after the third intermission bit of the
 *         first frame.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  Two different test frames are used for each of the two elementary tests.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frames.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_9 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            frm_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_kind_);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);
            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. In first Elementary test, Intermission lasts only 2 bits -> Remove last bit of
             *      intermission!
             *   2. Monitor frames as if received, driver frame must have ACK too!
             *************************************************************************************/
            if (elem_test.index_ == 1)
            {
                drv_bit_frm->RemoveBit(2, BitKind::Interm);
                mon_bit_frm->RemoveBit(2, BitKind::Interm);
            }

            mon_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            mon_bit_frm_2->ConvRXFrame();
            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            PushFramesToLT(*drv_bit_frm_2, *mon_bit_frm_2);
            RunLT(true, true);
            CheckLTResult();

            CheckRxFrame(*gold_frm);
            CheckRxFrame(*gold_frm_2);

            FreeTestObjects();
            return FinishElemTest();
        }
};