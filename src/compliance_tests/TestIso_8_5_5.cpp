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
 * @date 29.8.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.5
 *
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter does not transmit any frame before the end of the
 *        suspend transmission following an error frame.
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
 *   There is one elementarty test to perform:
 *      #1 After passive error flag, the LT forces the bus to recessive for error
 *         delimiter + intermission + suspend transmission time.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to send a passive error frame in data field
 *  according to elementary test cases.
 *
 * Response:
 *  After the intermission following the error frame, the LT verifies that the
 *  IUT wait 8 more bits before re-transmitting the frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_5 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1 , FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            dut_ifc->SetErrorState(FaultConfState::ErrPas);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            /* ESI needed for CAN FD variant */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, RtrFlag::Data,
                                                       EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force 7-th data bit to Dominant. This should cause stuff error.
             *   3. Insert Passive Error frame to both driven and monitored frames from next bit
             *      further.
             *   4. Append Suspend transmission to both driven and monitored frames.
             *   5. Append the same frame for second time to driven and monitored frames. In driven
             *      frame, it is as if received, in monitored as if transmitted.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            for (size_t i = 0; i < 8; i++)
            {
                drv_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
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