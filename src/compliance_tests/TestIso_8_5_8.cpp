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
 * @test ISO16845 8.5.8
 *
 * @brief The purpose of this test is to verify that a passive state IUT, after
 *        losing arbitration, repeats the frame without inserting any suspend
 *        transmission.
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
 *      #1 The LT causes the IUT to lose arbitration by sending a frame of
 *         higher priority.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test cases.
 *
 * Response:
 *  The LT verifies that the IUT re-transmits its frame (1 + 7 + 3) bit times
 *  after acknowledging the received frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_8 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            n_elem_tests = 1;
            AddElemTest(TestVariant::Common, ElemTest(1 , FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            dut_ifc->SetErrorState(FaultConfState::ErrPas);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* To avoid stuff bits causing mismatches betwen frame lengths */
            uint8_t data_byte = 0xAA;

            /* ESI needed for CAN FD variant */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, 0x44A, &data_byte);

            /* This frame should win arbitration */
            gold_frm_2 = std::make_unique<Frame>(*frm_flags, 0x1, 0x24A, &data_byte);
            RandomizeAndPrint(gold_frm.get());
            RandomizeAndPrint(gold_frm_2.get());

            /* This will be frame beating IUT with lower ID */
            drv_bit_frm = ConvBitFrame(*gold_frm_2);

            /* This is frame sent by IUT (ID= 0x200)*/
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* This is retransmitted frame by IUT */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Loose arbitration on monitored frame on first bit (0x245 vs 0x445 IDs).
             *   2. Compensate IUTs input delay, since next edge applied by LT will be perceived
             *      as shifted by IUT, due to its input delay.
             *   3. Do iteration specific compensation due to different number of stuff bits (since
             *      there are different IDs and CRCs):
             *        A. Common variant - remove one bit
             *   4. Append the same frame, retransmitted.
             *************************************************************************************/
            mon_bit_frm->LooseArbit(0, BitKind::BaseIdent);
            mon_bit_frm->GetBitOf(0, BitKind::BaseIdent)
                ->GetFirstTQIter(BitPhase::Sync)->Lengthen(dut_input_delay);

            if (test_variant == TestVariant::Common)
                mon_bit_frm->RemoveBit(mon_bit_frm->GetBitOf(0, BitKind::Data));

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