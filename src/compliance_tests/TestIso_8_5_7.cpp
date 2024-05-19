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
 * @test ISO16845 8.5.7
 *
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter does not transmit any frame before the end of the
 *        suspend transmission following a frame.
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
 *      #1 After the dominant ACK, the LT forces the bus to recessive for
 *         ACK delimiter + EOF + intermission + suspend transmission time.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit two frames.
 *  The LT lets the IUT transmit the first frame according to elementary test
 *  cases.
 *  This frame shall end with EOF field followed by the intermission field.
 *
 * Response:
 *  After the intermission following the first frame, the LT verifies that the
 *  IUT wait 8 more bits before transmitting the second frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_7 : public test::TestBase
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
            /* ESI needed for CAN FD variant */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* ESI needed for CAN FD variant */
            frm_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_kind_, EsiFlag::ErrPas);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Append suspend transmission field to both driven and monitored frames.
             *   3. Append next frame.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->CompensateEdgeForInputDelay(
                drv_bit_frm->GetBitOf(0, BitKind::Ack), dut_input_delay);

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
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            dut_ifc->SendFrame(gold_frm_2.get());
            WaitForDrvAndMon();

            CheckLTResult();

            return FinishElemTest();
        }

};