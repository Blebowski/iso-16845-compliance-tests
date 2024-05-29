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
 * @date 19.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.1
 *
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting a bit error during the transmission of an
 *        active error flag.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      FDF = 0
 *
 *  CAN FD Enabled
 *      FDF = 1
 *
 * Elementary test cases:
 *   Elementary tests to perform:
 *      #1 corrupting the first bit of the active error flag;
 *      #2 corrupting the third bit of the active error flag;
 *      #3 corrupting the sixth bit of the active error flag.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *
 *  Then, the LT corrupts a bit in data field to causes the IUT to generate an
 *  active error frame.
 *
 *  The LT corrupts one of the dominant bits of the error flag according to
 *  elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be increased by 8 on the corrupted bit.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_1 : public test::TestBase
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

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* Second frame the same due to retransmission. */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force 7-th data bit to dominant to cause stuff error.
             *   2. Insert Active Error frame from next bit on.
             *   3. Corrupt 1,3,6 th bit of Active Error flag, flip to opposite value!
             *   4. Insert next Active Error frame from next bit on.
             *   5. Append the same frame after the first frame since IUT will retransmitt the
             *      frame. Force ACK low on driven frame.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertActErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);

            size_t bit_index_to_corrupt;
            if (elem_test.index_ == 1)
                bit_index_to_corrupt = 0;
            else if (elem_test.index_ == 2)
                bit_index_to_corrupt = 2;
            else
                bit_index_to_corrupt = 5;

            Bit *bit_to_corrupt = drv_bit_frm->GetBitOf(bit_index_to_corrupt,
                                                            BitKind::ActErrFlag);
            bit_to_corrupt->val_ = BitVal::Recessive;

            size_t bit_index = drv_bit_frm->GetBitIndex(bit_to_corrupt);
            drv_bit_frm->InsertActErrFrm(bit_index + 1);
            mon_bit_frm->InsertActErrFrm(bit_index + 1);

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            /* 8 for first Error frame, 8 for next one, - 1 for succesfull retransmission */
            CheckTecChange(tec_old, 15);

            return FinishElemTest();
        }

};