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
 * @test ISO16845 8.3.1
 *
 * @brief This test verifies that an IUT acting as a transmitter tolerates up
 *        to 7 dominant bits after sending its own error flag.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *          FDF = 0
 *      CAN FD Enabled
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 the LT extends the error flag by 1 dominant bit;
 *          #2 the LT extends the error flag by 4 dominant bits;
 *          #3 the LT extends the error flag by 7 dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. The LT corrupts this frame in
 *  data field causing the IUT to send an active error frame. The LT prolongs
 *  the error flag sent by IUT according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate only one error frame.
 *  The IUT shall restart the transmission after the intermission field
 *  following the error frame.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_3_1 : public test::TestBase
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
             *  4. Insert 1,4,7 dominant bits to driven frame after active error flag in driven
             *     frame (prolong error flag). Insert equal amount of recessive bits to monitored
             *     frame (this corresponds to accepting longer Error flag without re-sending next
             *     error flag).
             *  5. Append the same frame second time. This checks retransmission.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(6, BitKind::Data)->val_ = BitVal::Dominant;

            size_t bit_index = drv_bit_frm->GetBitIndex(drv_bit_frm->GetBitOf(7, BitKind::Data));
            drv_bit_frm->InsertActErrFrm(bit_index);
            mon_bit_frm->InsertActErrFrm(bit_index);

            size_t bits_to_insert;
            if (elem_test.index_ == 1)
                bits_to_insert = 1;
            else if (elem_test.index_ == 2)
                bits_to_insert = 4;
            else
                bits_to_insert = 7;

            Bit *first_err_delim_bit = drv_bit_frm->GetBitOf(0, BitKind::ErrDelim);
            size_t first_err_delim_index = drv_bit_frm->GetBitIndex(first_err_delim_bit);

            for (size_t k = 0; k < bits_to_insert; k++)
            {
                drv_bit_frm->InsertBit(BitKind::ActErrFlag, BitVal::Dominant, first_err_delim_index);
                mon_bit_frm->InsertBit(BitKind::PasErrFlag, BitVal::Recessive, first_err_delim_index);
            }

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
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};