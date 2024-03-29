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
 * @date 17.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.7
 *
 * @brief This test verifies that the IUT increases its REC by 1 when detecting
 *        a form error on ACK delimiter.
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
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      #1 Classical CAN frame (ACK = 0, ACK delimiter = 0)
 *
 *  CAN FD Enabled
 *    In FD format, 2 ACK bits are used to check the second possible ACK bit
 *    position (delayed ACK).
 *      #1 FD frame (ACK bit 1 = 0; ACK bit 2 = 0, ACK delimiter = 0)
 *
 * Setup:
 *  The IUT is left in the default state.
 *  The LT sends a frame with a stuff error in it and force 1 bit of error flag
 *  to recessive.
 *  This initializes the REC counter to 1 + 8 REC = 9.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 on the dominant ACK delimiter.
 *  The REC value shall be decreased by 1 because the frame is error free until
 *  ACK.
 *  The REC value shall be unchanged as previous initialized while set up.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_7 : public test::TestBase
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
             *   2. Set ACK bit to dominant in driven frame. In CAN FD Enabled variant, flip both
             *      ACK bits to dominant!
             *   3. Flip ACK delimiter in driven frame (on can_tx) to DOMINANT!
             *   4. Insert expected Active error frame from first bit of EOF!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            if (test_variant == TestVariant::CanFdEna)
                drv_bit_frm->GetBitOf(1, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->GetBitOf(0, BitKind::AckDelim)->val_ = BitVal::Dominant;

            drv_bit_frm->InsertActErrFrm(0, BitKind::Eof);
            mon_bit_frm->InsertActErrFrm(0, BitKind::Eof);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            /* Dont use extra frame, but preset REC directly -> Simpler */
            dut_ifc->SetRec(9);
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);

            CheckLTResult();
            CheckRecChange(rec_old, +0);

            FreeTestObjects();
            return FinishElemTest();
        }
};