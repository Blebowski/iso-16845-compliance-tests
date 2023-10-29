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
 * @date 27.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.17
 *
 * @brief This test verifies that a passive state IUT acting as a transmitter
 *        does not increase its TEC when detecting an acknowledgement error
 *        followed by a passive error flag.
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
 *     #1 ACK = recessive
 *
 * Setup:
 *  The IUT is set to the TEC passive.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT sends acknowledgement for this frame according to elementary
 *  test cases.
 *  After the acknowledgement error, the LT sends a passive error frame.
 *
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_17 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            dut_ifc->SetTec((rand() % 125) + 130);
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
             *   3. Insert Passive Error Frame to both driven and monitored frames from ACK deli-
             *      miter further.
             *   4. Append suspend transmission since IUT is Error passive!
             *   5. Insert retransmitted frame, but with ACK set.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            drv_bit_frm->InsertPasErrFrm(0, BitKind::AckDelim);
            mon_bit_frm->InsertPasErrFrm(0, BitKind::AckDelim);

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
            tec_old = dut_ifc->GetTec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();
            /* +0 for ACK Error, -1 for succesfull retransmission! */
            CheckTecChange(tec_old, -1);

            return FinishElemTest();
        }

};