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
 * @test ISO16845 7.5.1
 *
 * @brief The purpose of this test is to verify that an error passive IUT
 *        considers the passive error flag as completed after the detection
 *        of 6 consecutive bits of the same value.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Passive Error flag, FDF = 0
 *
 *  CAN FD Enabled
 *      Passive Error flag, FDF = 1
 *
 * Elementary test cases:
 *  Elementary tests to perform:
 *      #1 superimposing the passive error flag by an active error flag
 *         starting at the first bit;
 *      #2 superimposing the passive error flag by an active error flag
 *         starting at the third bit;
 *      #3 superimposing the passive error flag by an active error flag
 *         starting at the sixth bit.
 *
 * Setup:
 *  The IUT is set in passive state.
 *
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  During the passive error flag sent by the IUT, the LT sends an active
 *  error flag according to elementary test cases.
 *  At the end of the active error flag, the LT waits for (8 + 2) bit time
 *  before sending a valid test frame.
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

class TestIso_7_5_1 : public test::TestBase
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

            dut_ifc->SetTec(140);
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
             *   4. Superimpose active error flag on driven frame starting from 1/3/6th bit of
             *      Passive Error flag. On monitored frame, insert passive error frame again. This
             *      corresponds prolonging passive error flag until sequence of equal consecutive
             *      bits is received!
             *   5. Remove last bit of intermission in driven frame. This corresponds to +2 bits
             *      separation in test description.
             *   6. Turn monitored frame as if received, remove SOF since frame is transmitted by
             *      LT after second bit of intermission.
             *   7. Append the second frame to original frame. Second driven frame must have ACK
             *      dominant since TX/RX feedback is disabled!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            int bit_to_corrupt;
            if (elem_test.index_ == 1)
                bit_to_corrupt = 0;
            else if (elem_test.index_ == 2)
                bit_to_corrupt = 2;
            else
                bit_to_corrupt = 5;

            drv_bit_frm->InsertActErrFrm(bit_to_corrupt, BitKind::PasErrFlag);
            mon_bit_frm->InsertPasErrFrm(bit_to_corrupt, BitKind::PasErrFlag);

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
            CheckNoRxFrame(); /* Only one frame should be received */

            FreeTestObjects();
            return FinishElemTest();
        }
};