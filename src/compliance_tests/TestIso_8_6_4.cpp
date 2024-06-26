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
 * @date 20.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.4
 *
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting 8 consecutive dominant bits following the
 *        transmission of its passive error flag and after each sequence of
 *        additional 8 consecutive dominant bits.
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
 *   After the error flag sent by the IUT, the LT sends a sequence of up to 16
 *   dominant bits.
 *      There are five elementary tests to perform:
 *          #1 dominant bits after passive error flag: 1 bit;
 *          #2 dominant bits after passive error flag: 6 bits;
 *          #3 dominant bits after passive error flag: 8 bits;
 *          #4 dominant bits after passive error flag: 9 bits;
 *          #5 dominant bits after passive error flag: 16 bits.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT corrupts a bit in data field to cause the IUT to generate a
 *  passive error frame.
 *  After the error flag sent by the IUT, the LT sends a sequence according to
 *  elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be increased by 8 on each eighth dominant bit
 *  after the error flag.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 5; i++)
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
            uint8_t data_byte = 0x80;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
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
             *   2. Insert Passive Error frame from next bit on to monitored frame. Insert Passive
             *      Error frame to driven frame.
             *   3. 1,6,8,9,16 dominant bits after passive error flag to driven frame! Insert the
             *      same amount of recessive bits to monitored frame.
             *   4. Append suspend transmission
             *   5. Append retransmitted frame!
             *************************************************************************************/
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            size_t num_bits_to_insert = 0;
            switch (elem_test.index_)
            {
            case 1:
                num_bits_to_insert = 1;
                break;
            case 2:
                num_bits_to_insert = 6;
                break;
            case 3:
                num_bits_to_insert = 8;
                break;
            case 4:
                num_bits_to_insert = 9;
                break;
            case 5:
                num_bits_to_insert = 16;
                break;
            default:
                break;
            }

            for (size_t i = 0; i < num_bits_to_insert; i++)
            {
                size_t bit_index = drv_bit_frm->GetBitIndex(
                                        drv_bit_frm->GetBitOf(5, BitKind::PasErrFlag));
                drv_bit_frm->InsertBit(BitKind::ActErrFlag, BitVal::Dominant, bit_index + 1);
                mon_bit_frm->InsertBit(BitKind::PasErrFlag, BitVal::Recessive, bit_index + 1);
            }

            // Compensate first dominant driven by to account for IUTs input delay
            drv_bit_frm->CompensateEdgeForInputDelay(
                drv_bit_frm->GetBitOf(0, BitKind::ActErrFlag), dut_input_delay);

            for (size_t i = 0; i < 8; i++)
            {
                drv_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
            }

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetTec(130); // Preset each time to avoid going bus-off
            tec_old = dut_ifc->GetTec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            /* Based on number of inserted bits:
             *  1,6 bits -> 8 bits not reached. Only +8 for first error frame
             *  8,9 bits -> 8 bits reached. +16 (first error frame and +8)
             *  16 bits  -> 2 * 8 bits reached (+24, first error frame and 2 * 8 bits)
             *  -1 always decrement for succesfull retransmission
             */
            if (elem_test.index_ == 1 || elem_test.index_ == 2)
                CheckTecChange(tec_old, 7);
            else if (elem_test.index_ == 3 || elem_test.index_ == 4)
                CheckTecChange(tec_old, 15);
            else
                CheckTecChange(tec_old, 23);

            return FinishElemTest();
        }

};