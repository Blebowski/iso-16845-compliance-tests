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
 * @test ISO16845 7.2.9
 *
 * @brief This test verifies that the IUT detects a form error when the
 *        recessive ACK delimiter is forced to dominant state by LT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ACK delimiter, ACK = 0, FDF = 0
 *
 *  CAN FD Enabled
 *      ACK delimiter, ACK1 = 0, ACK2 = 0, FDF = 1
 *
 * Elementary test cases:
 *      #1 ACK delimiter = 0
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with form error at ACK delimiter according to
 *  elementary test cases.
 *
 * Response:
 *  The IUT shall generate an active error frame at the bit position following
 *  the ACK delimiter.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_2_9 : public test::TestBase
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
             *   1. Monitor frame as if received.
             *   2. CAN 2.0 Variant -> Force ACK Delimiter DOMINANT
             *      CAN FD Variant  -> Force second ACK bit DOMINANT
             *                      -> Force ACK Delimiter DOMINANT
             *   3. Insert Active Error frame from first bit of EOF!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            if (test_variant == TestVariant::CanFdEna)
                drv_bit_frm->GetBitOf(1, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->GetBitOf(0, BitKind::AckDelim)->val_ = BitVal::Dominant;

            mon_bit_frm->InsertActErrFrm(0, BitKind::Eof);
            drv_bit_frm->InsertActErrFrm(0, BitKind::Eof);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};