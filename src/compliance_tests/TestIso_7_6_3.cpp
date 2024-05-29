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
 * @date 16.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.3
 *
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        the eighth consecutive dominant bit following the transmission of its
 *        active error flag and after each sequence of additional 8 consecutive
 *        dominant bits.
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
 *      #1 16 bit dominant
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  After the error flag sent by the IUT, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 8 on each eighth dominant bit
 *  after the error flag.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_3 : public test::TestBase
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
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                            RtrFlag::Data);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &error_data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit! This will
             *      cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Insert 16 Dominant bits directly after Error frame (from first bit of Error
             *      Delimiter). These bits shall be driven on can_tx, but 16 RECESSIVE bits shall
             *      be monitored on can_tx.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);
            drv_bit_frm->InsertActErrFrm(7, BitKind::Data);

            Bit *err_delim = drv_bit_frm->GetBitOf(0, BitKind::ErrDelim);
            size_t bit_index = drv_bit_frm->GetBitIndex(err_delim);

            for (size_t k = 0; k < 16; k++)
            {
                drv_bit_frm->InsertBit(BitKind::ActErrFlag, BitVal::Dominant, bit_index);
                mon_bit_frm->InsertBit(BitKind::ActErrFlag, BitVal::Recessive, bit_index);
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckNoRxFrame();

            /*
             * Check that REC has incremented by 25
             *  +1 for original error frame,
             *  +8 for detecting dominant bit first bit after error flag.
             *  +2*8 for each 8 dominant bits after Error flag! after each error flag!
             */
            CheckRecChange(rec_old, +25);

            FreeTestObjects();
            return FinishElemTest();
        }
};