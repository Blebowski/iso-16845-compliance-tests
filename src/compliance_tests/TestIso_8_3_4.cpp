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
 * @date 26.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.3.4
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a form
 *        error when it receives an invalid error delimiter.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          FDF = 0
 *      CAN FD Enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 corrupting the second bit of the error delimiter;
 *          #2 corrupting the fourth bit of the error delimiter;
 *          #3 corrupting the seventh bit of the error delimiter.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT corrupts this frame in data field causing the IUT to send an active
 *  error frame.
 *  Then, the LT forces 1 recessive bit of the error delimiter to the dominant
 *  state according to elementary test cases.
 *
 * Response:
 *  The IUT shall restart the error frame at the bit position following the
 *  corrupted bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_3_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (int i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80; // 7-th data bit will be recessive stuff bit

            if (test_variant == TestVariant::Common)
                frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, RtrFlag::Data);
            else
                frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, EsiFlag::ErrAct);

            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Turn driven frame as received.
             *  2. Flip 7-th data bit of driven frame to dominant, this will destroy recessive
             *     stuff bit send by IUT.
             *  3. Insert expected active error frame from 8-th bit of data field to monitored
             *     frame. Insert the same to driven frame.
             *  4. Flip 2,4 or 7-th bit of Error delimiter to Dominant. Insert next expected error
             *     frame from one bit further.
             *  5. Turn second driven frame (the same) as received. Append after first frame. This
             *     checks retransmission.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(6, BitKind::Data)->val_ = BitVal::Dominant;

            drv_bit_frm->InsertActErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);

            int bit_index_to_flip;
            if (elem_test.index_ == 1)
                bit_index_to_flip = 2;
            else if (elem_test.index_ == 2)
                bit_index_to_flip = 4;
            else
                bit_index_to_flip = 7;

            Bit *bit_to_flip = drv_bit_frm->GetBitOf(bit_index_to_flip - 1,
                                                        BitKind::ErrDelim);
            drv_bit_frm->FlipBitAndCompensate(bit_to_flip, dut_input_delay);

            int next_error_flag_index = drv_bit_frm->GetBitIndex(bit_to_flip) + 1;

            drv_bit_frm->InsertActErrFrm(next_error_flag_index);
            mon_bit_frm->InsertActErrFrm(next_error_flag_index);

            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            this->dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};