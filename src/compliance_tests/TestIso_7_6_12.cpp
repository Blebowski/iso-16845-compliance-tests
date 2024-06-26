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
 * @date 18.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.12
 *
 * @brief This test verifies that a receiver increases its REC by 1 when
 *        detecting a form error on a bit of the error delimiter it is
 *        transmitting.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 *
 *  CAN FD Enabled
 *      REC, FDF = 1
 *
 * Elementary test cases:
 *      #1 the second bit of the error delimiter is corrupted;
 *      #2 the seventh bit of the error delimiter is corrupted.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  The LT corrupts 1 bit of the error delimiter according to elementary
 *  test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 after reception of the
 *  dominant bit sent by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_12 : public test::TestBase
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

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, RtrFlag::Data);
            gold_frm = std::make_unique<Frame>(*frm_flags, 1, &error_data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit! This will
             *      cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Flip 2nd or 7th bit of Error delimiter to dominant!
             *   5. Insert next active error frame from 3-rd or 8-th bit of Error delimiter!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);
            drv_bit_frm->InsertActErrFrm(7, BitKind::Data);

            /* Force n-th bit of Error Delimiter to dominant! */
            size_t bit_to_corrupt;
            if (elem_test.index_ == 1)
                bit_to_corrupt = 2;
            else
                bit_to_corrupt = 7;

            TestMessage("Forcing Error delimiter bit %zu to Dominant", bit_to_corrupt);
            Bit *bit = drv_bit_frm->GetBitOf(bit_to_corrupt - 1, BitKind::ErrDelim);
            bit->val_ = BitVal::Dominant;

            mon_bit_frm->InsertActErrFrm(bit_to_corrupt, BitKind::ErrDelim);
            drv_bit_frm->InsertActErrFrm(bit_to_corrupt, BitKind::ErrDelim);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            /* 1 stuff error in data field, 1 form error in error delimiter! */
            CheckRecChange(rec_old, +2);

            FreeTestObjects();
            return FinishElemTest();
        }
};