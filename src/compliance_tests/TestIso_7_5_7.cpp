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
 * @date 30.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.5.7
 *
 * @brief The purpose of this test is to verify that an IUT changes its state
 *        from active to passive.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Error at error frame, FDF = 0
 *
 *  CAN FD Enabled
 *      Error at error frame, FDF = 1
 *
 * Elementary test cases:
 *  There is one test to perform.
 *      #1 Bit error up to REC passive limit by sending 17 recessive bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  The LT corrupts the following active error flag according to elementary
 *  test cases. After this sequence, the IUT shall be error passive and sending
 *  a passive error flag.
 *  The LT send a valid frame 6 + 8 + 3 bit after dominant part of previous
 *  error sequence.
 *
 * Response:
 *  The IUT shall generate a passive error flag starting at the bit position
 *  following the last recessive bit sent by the LT.
 *  The IUT shall acknowledge the following test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_5_7 : public test::TestBase
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
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
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
             *   3. Remove all bits from next bit on.
             *   4. Append 17 recessive bits to driven frame and 17 dominant bits to monitored
             *      frame. This corresponds to retransmissions of active error flag by IUT.
             *   5. Append Passive Error frame to monitored frame and also to driven frame (this
             *      also includes Intermission)
             *   6. Append next frame as if received by IUT.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->RemoveBitsFrom(7, BitKind::Data);
            mon_bit_frm->RemoveBitsFrom(7, BitKind::Data);

            /*
             * We need to insert 18 since following insertion of passive error frame over
             * writes bit from which error frame starts!
             */
            for (int i = 0; i < 18; i++)
            {
                drv_bit_frm->AppendBit(BitKind::ActErrFlag, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::PasErrFlag, BitVal::Dominant);
            }

            int last_bit = drv_bit_frm->GetLen();
            drv_bit_frm->InsertPasErrFrm(last_bit - 1);
            mon_bit_frm->InsertPasErrFrm(last_bit - 1);

            mon_bit_frm_2->ConvRXFrame();
            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(0); /* Must be reset before every elementary test */
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);
            CheckNoRxFrame(); /* Only one frame should be received! */

            FreeTestObjects();
            return FinishElemTest();
        }
};