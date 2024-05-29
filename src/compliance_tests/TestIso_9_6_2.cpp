/*****************************************************************************
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
 * @date 14.2.2021
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 9.6.2
 *
 * @brief This test verifies that increasing REC and TEC are independent operations.
 * @version CAN FD enabled
 *
 * Test variables:
 *     Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          REC
 *          TEC
 *          FDF = 0
 *
 *     CAN FD Enabled:
 *          REC
 *          TEC
 *          FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to increase its TEC up to 127. Then, LT causes the
 *  IUT to increase its REC up to 128. Then, the LT sends a frame containing
 *  a stuff error in data field.
 *
 * Response:
 *  Each increment of the REC shall be responded by an active error flag.
 *  The IUT responds to the stuff error with a passive error flag.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_9_6_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);

            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            // This test has IUT as receiver, so no trigger/waiting config is needed!
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                    RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            frm_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                    RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);

            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, 0xAA, &data_byte);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2, 0x1, 0xAA, &data_byte);
            gold_frm->Print();

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            // Separate frame is needed for CAN FD enabled variant. This frame is already with
            // IUT being Error passive, so we need frame/frame_flags with ESI error passive!
            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip monitored frame as if received.
             *   2. Flip 7-th bit of data field. This should bit stuff bit. Do this in both frames.
             *   3. Insert Active Error frame to Monitored frame from next bit. Insert Active
             *      Error frame to driven frame (TX/RX feedback disabled). Do this in both frames.
             *
             *      This first frame will be transmitted 128 times. This accounts for incrementing
             *      REC to 128.
             *
             *   4. In second frame, turn monitored frame as if received.
             *   5. In driven frame, flip 7-th bit of data field to cause stuff error.
             *   6. Insert Passive Error frame to both driven and monitored frames.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertActErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);

            mon_bit_frm_2->ConvRXFrame();
            drv_bit_frm_2->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm_2->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm_2->InsertPasErrFrm(7, BitKind::Data);

            TestMessage("First frame");
            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            TestMessage("Second frame");
            drv_bit_frm_2->Print(true);
            mon_bit_frm_2->Print(true);

            /**************************************************************************************
             * Execute test
             *  1. Preset TEC to 127.
             *  2. Try to send frame with error in it 128 times by LT. This should increment
             *     IUTs REC to 128 and IUT will just become error passive.
             *  3. Send one more frame with stuff error in it. This-one should have passive error
             *     frame as response.
             *************************************************************************************/
            dut_ifc->SetTec(127);
            dut_ifc->SetRec(0);

            for (size_t i = 0; i < 128; i++)
            {
                TestMessage("Sending frame nr. : %zu", i);
                int rec_old = dut_ifc->GetRec();
                PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
                RunLT(true, true);
                CheckLTResult();
                CheckRecChange(rec_old, +1);
            }

            TestMessage("Sending frame which should lead to passive error flag!");
            PushFramesToLT(*drv_bit_frm_2, *mon_bit_frm_2);
            RunLT(true, true);
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};