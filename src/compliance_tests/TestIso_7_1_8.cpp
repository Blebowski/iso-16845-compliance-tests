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
 * @test ISO16845 7.1.8
 *
 * @brief This test verifies the behaviour of the IUT when receiving a correct
 *        classical frame with a DLC greater than 8.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  DLC, FDF = 0
 *
 * Elementary test cases:
 *  There are seven elementary tests, for which DLC ∈ [9h , Fh].
 *
 *      TEST    DLC
 *       #1     0x9
 *       #2     0xA
 *       #3     0xB
 *       #4     0xC
 *       #5     0xD
 *       #6     0xE
 *       #7     0xF
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data and DLC received by the IUT during the test state shall match the
 *  data and DLC sent in the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_8 : public test::TestBase
{
    public:

        uint8_t dlcs[7] = {0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            for (size_t i = 0; i < 7; i++)
                 AddElemTest(TestVariant::Common, ElemTest(i + 1));
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlcs[elem_test.index_ - 1]);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received, driver frame must have ACK too (TX->RX feedback
             *      disabled).
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

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