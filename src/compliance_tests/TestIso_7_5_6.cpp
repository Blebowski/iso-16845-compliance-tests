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
 * @date 29.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.5.6
 *
 * @brief The purpose of this test is to verify that an error passive IUT
 *        detects a form error when receiving an invalid error delimiter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Error delimiter of passive error frame, FDF = 0
 *
 *  CAN FD Enabled
 *      Error delimiter of passive error frame, FDF = 1
 *
 * Elementary test cases:
 *  Elementary tests to perform:
 *      #1 corrupting the second bit of the error delimiter;
 *      #2 corrupting the fourth bit of the error delimiter;
 *      #3 corrupting the seventh bit of the error delimiter.
 *
 * Setup:
 *  The IUT is set in passive state.
 *
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  During the error delimiter, the LT creates a form error according to
 *  elementary test cases.
 *  After the form error, the LT waits for (6 + 7) bit time before sending
 *  a dominant bit, corrupting the last bit of the error delimiter.
 *
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the last dominant bit sent by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_5_6 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            dut_ifc->SetTec((rand() % 110) + 128);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                            IdentKind::Base, RtrFlag::Data, BrsFlag::NoShift,
                            EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &error_data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff bit
             *      therefore causing error.
             *   3. Insert Passive Error frame to both driven and monitored frames from next bit.
             *   4. Corrupt 2/4/7-th bit of Error delimiter to dominant on driven frame.
             *   5. Insert next error frame from next bit on. Both driven and monitored frames
             *      contain passive error frame.
             *   6. Flip last bit (8-th) of error delimiter of new error frame to dominant.
             *   7. Insert overload frame to both driven and monitored frames (TX/RX feedback is
             *      disabled).
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            size_t bit_to_corrupt;
            if (elem_test.index_ == 1)
                bit_to_corrupt = 1;
            else if (elem_test.index_ == 2)
                bit_to_corrupt = 3;
            else
                bit_to_corrupt = 6;

            Bit *corrupted_bit = drv_bit_frm->GetBitOf(bit_to_corrupt,
                                    BitKind::ErrDelim);
            size_t bit_index = drv_bit_frm->GetBitIndex(corrupted_bit);
            corrupted_bit->val_ = BitVal::Dominant;

            drv_bit_frm->InsertPasErrFrm(bit_index + 1);
            mon_bit_frm->InsertPasErrFrm(bit_index + 1);

            /* This should be last bit of second Error delimiter*/
            drv_bit_frm->GetBit(bit_index + 14)->val_ = BitVal::Dominant;

            drv_bit_frm->InsertOvrlFrm(bit_index + 15);
            mon_bit_frm->InsertOvrlFrm(bit_index + 15);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckNoRxFrame();

            FreeTestObjects();
            return FinishElemTest();
        }
};