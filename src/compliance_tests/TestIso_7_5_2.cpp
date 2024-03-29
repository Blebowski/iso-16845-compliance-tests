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
 * @date 28.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.5.2
 *
 * @brief The purpose of this test is to verify that an error passive IUT
 *        accepts a frame starting after the second bit of the intermission
 *        following the error frame it has transmitted.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Intermission field of passive error frame, FDF = 0
 *
 *  CAN FD Enabled
 *      Intermission field of passive error frame, FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform:
 *      #1 Error delimiter + intermission − 1 (8 + 2 bit time)
 *
 * Setup:
 *  The IUT is set in passive state.
 *
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  At the end of the passive error flag, the LT waits according to elementary
 *  test cases before sending a valid test frame.
 *
 * Response:
 *  The IUT shall acknowledge the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_5_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

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

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff bit
             *      therefore causing error.
             *   3. Insert Passive Error frame to both driven and monitored frames from next bit.
             *   4. Remove last bit of Intermission from driven frame.
             *   5. Remove SOF from retransmitted frame (reception after second bit of
             *      intermission) in monitored frame.
             *   6. Append retransmitted frame with ACK set (TX/RX feedback disabled!)
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            drv_bit_frm->RemoveBit(2, BitKind::Interm);

            mon_bit_frm_2->ConvRXFrame();
            mon_bit_frm_2->RemoveBit(0, BitKind::Sof);

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            CheckRxFrame(*gold_frm);
            CheckNoRxFrame(); /* Only one frame should be received! */

            FreeTestObjects();
            return FinishElemTest();
        }
};