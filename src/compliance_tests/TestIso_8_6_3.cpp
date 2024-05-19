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
 * @test ISO16845 8.6.3
 *
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting 8 consecutive dominant bits following the
 *        transmission of its active error flag and after each sequence of
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
 *   Elementary tests to perform:
 *      #1 After the error flag sent by the IUT, the LT sends a sequence of 16
 *         dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an active error frame in data field
 *  according to elementary test cases.
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
#include "../can_lib/BitTiming.h"

using namespace can;
using namespace test;

class TestIso_8_6_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
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
             *   2. Insert Active Error frame from next bit on to monitored frame. Insert Passive
             *      Error frame to driven frame.
             *   3. Insert 16 dominant bits to driven frame after the end of error flag. Insert 16
             *      recessive bits to monitored frame.
             *   4. Append retransmitted frame!
             *************************************************************************************/
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);

            for (size_t i = 0; i < 16; i++)
            {
                size_t bit_index = drv_bit_frm->GetBitIndex(
                                        drv_bit_frm->GetBitOf(5, BitKind::PasErrFlag));
                drv_bit_frm->InsertBit(BitKind::ActErrFlag, BitVal::Dominant, bit_index + 1);
                mon_bit_frm->InsertBit(BitKind::PasErrFlag, BitVal::Recessive, bit_index + 1);
            }

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
            /* 8 for first Error frame, 2 * 8 for 16 bits, - 1 for succesfull retransmission */
            CheckTecChange(tec_old, 23);

            return FinishElemTest();
        }

};