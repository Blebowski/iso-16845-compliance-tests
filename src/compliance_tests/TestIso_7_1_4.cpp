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
 * @date 3.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.4
 *
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid base
 *        format frame.
 *
 * @version CAN FD Enabled, Classical CAN
 *
 * Test variables:
 *  Classical CAN  : FDF = 1
 *  CAN FD Enabled : FDF = 1, RRS = 1
 *
 * Elementary test cases:
 *  Classical CAN:
 *      #1 FDF = 1
 *
 *  CAN FD Enabled:
 *      #2 RRS = 1
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *
 * @todo: Classical CAN version not supported!
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::ClasCanAndFdEna);
            if (test_variants.size() > 0)
            {
                if (test_variants[0] == TestVariant::Can20)
                    AddElemTest(TestVariant::Can20, ElemTest(1, FrameKind::Can20));
                else
                    AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force bit as given by elementary test to recessive.
             *   2. Update frames since by sending different bit value, CRC might have changed.
             *   3. Monitor frame as if received (IUT is receiving)
             *************************************************************************************/
            if (test_variant == TestVariant::Can20)
            {
                /* When node is "Classical CAN" conformant, it shall accept recessive R0 (FDF) and
                 * continue without protocol exception or regarding this frame type as FD Frame!!
                 */
                drv_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
                mon_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
            } else {
                /* R1 bit corresponds to RRS in CAN FD frames. It is bit on position of RTR in
                 * CAN2.0 frames!
                 */
                drv_bit_frm->GetBitOf(0, BitKind::R1)->val_ = BitVal::Recessive;
                mon_bit_frm->GetBitOf(0, BitKind::R1)->val_ = BitVal::Recessive;
            }

            drv_bit_frm->UpdateFrame();
            mon_bit_frm->UpdateFrame();

            mon_bit_frm->ConvRXFrame();

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            FreeTestObjects();
            return FinishElemTest();
        }
};