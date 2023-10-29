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
 * @date 6.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.12
 *
 * @brief The purpose of this test is to verify the point of time at which a
 *        message is taken to be valid by the IUT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      EOF, FDF = 0
 *
 *  CAN FD Enabled
 *      EOF, FDF = 1
 *
 * Elementary test cases:
 *      #1 The last bit of the EOF is forced to dominant state.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The IUT shall generate an overload frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_12 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force last bit of EOF of driven frame to 0 (Overload condition)
             *   2. Put ACK to driven frame too!
             *   3. Turn monitored frame to received frame!
             *   4. Insert overload frame to monitored/driven frame on first bit of intermission.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(6, BitKind::Eof)->val_ = BitVal::Dominant;

            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            mon_bit_frm->ConvRXFrame();

            mon_bit_frm->InsertOvrlFrm(0, BitKind::Interm);
            drv_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

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