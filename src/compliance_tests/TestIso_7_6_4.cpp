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
 * @test ISO16845 7.6.4
 *
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        the eighth consecutive dominant bit following the transmission of its
 *        overload flag and after each sequence of additional 8 consecutive
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
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  After the overload flag sent by the IUT, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 8 on each eighth dominant bit
 *  after the overload flag.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_4 : public test::TestBase
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
             *   2. Force last bit of EOF to Dominant!
             *   3. Insert Overload frame from first bit of Intermission.
             *   4. Insert 16 Dominant bits directly after Overload frame (from first bit
             *      of Overload Delimiter). These bits shall be driven on can_tx, but 16
             *      RECESSIVE bits shall be monitored on can_tx.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->GetBitOf(6, BitKind::Eof)->val_ = BitVal::Dominant;

            mon_bit_frm->InsertOvrlFrm(0, BitKind::Interm);
            drv_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

            Bit *ovr_delim = drv_bit_frm->GetBitOf(0, BitKind::OvrlDelim);
            size_t bit_index = drv_bit_frm->GetBitIndex(ovr_delim);

            for (size_t k = 0; k < 16; k++)
            {
                drv_bit_frm->InsertBit(BitKind::OvrlFlag, BitVal::Dominant, bit_index);
                mon_bit_frm->InsertBit(BitKind::OvrlFlag, BitVal::Recessive, bit_index);
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

            /*
             * Receiver will make received frame valid on 6th bit of EOF! Therefore at
             * point where Error occurs, frame was already received OK and should be
             * readable!
             */
            CheckRxFrame(*gold_frm);

            /*
             * For first iteration we start from 0 so there will be no decrement on
             * sucessfull reception! For further increments there will be alway decrement
             * by 1 and increment by 2*8.
             */
            if (test_variant == TestVariant::Common)
                CheckRecChange(rec_old, +16);
            else
                CheckRecChange(rec_old, +15);

            FreeTestObjects();
            return FinishElemTest();
        }
};