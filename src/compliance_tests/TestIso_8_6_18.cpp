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
 * @test ISO16845 8.6.18
 *
 * @brief This test verifies that a passive state IUT acting as a transmitter
 *        increases its TEC when detecting an acknowledgement error followed
 *        by at least 1 dominant bit during the passive error flag.
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
 *     #1 LT send dominant bit at the sixth bit position of the passive error
 *        flag.
 *
 * Setup:
 *  The IUT is set to the TEC passive.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT does not acknowledge this frame.
 *  After the acknowledgement error, the LT sends a dominant bit according to
 *  elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value increased by 8 for
 *  the error in passive error flag.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_18 : public test::TestBase
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
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force ACK to recessive value.
             *   3. Insert Passive Error Frame to both driven and monitored frames from ACK
             *      delimiter further.
             *   4. Force last bit of Error flag to dominant. This shall lead to increment of TEC!
             *      This directly tests exception 1 of rule "c" of "Error counting" in
             *      ISO11898-1 2015!
             *   5. Insert next Passive Error frame one bit after it was flipped since passive
             *      error flag is only complete after 6 bits of equal polarity!
             *   6. Append suspend transmission since IUT is Error passive!
             *   7. Insert retransmitted frame, but with ACK set.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            drv_bit_frm->InsertPasErrFrm(0, BitKind::AckDelim);
            mon_bit_frm->InsertPasErrFrm(0, BitKind::AckDelim);

            Bit *last_err_flg_bit = drv_bit_frm->GetBitOf(5, BitKind::PasErrFlag);
            drv_bit_frm->FlipBitAndCompensate(last_err_flg_bit, dut_input_delay);

            size_t bit_index = drv_bit_frm->GetBitIndex(last_err_flg_bit);
            drv_bit_frm->InsertPasErrFrm(bit_index + 1);
            mon_bit_frm->InsertPasErrFrm(bit_index + 1);

            drv_bit_frm->AppendSuspTrans();
            mon_bit_frm->AppendSuspTrans();

            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetErrorState(FaultConfState::ErrPas);
            tec_old = dut_ifc->GetTec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            /*
             * +8 for ACK error followed by dominant bit during passive error flag,
             * -1 for succesfull retransmission!
             */
            CheckTecChange(tec_old, +7);

            return FinishElemTest();
        }

};