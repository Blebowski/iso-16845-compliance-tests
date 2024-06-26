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
 * @test ISO16845 8.5.2
 *
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter accepts to receive a frame starting after the second
 *        bit of the intermission following the error frame it has transmitted.
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
 *   There is one elementary test to perform:
 *      #1 LT waits for (8 + 2) bit time before sending a frame.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to send a passive error flag in data field.
 *  During the passive error flag sent by the IUT, the LT sends an active error
 *  flag.
 *  At the end of the error flag, the LT send a valid frame according to
 *  elementary test cases.
 *
 * Response:
 *  The IUT shall acknowledge the last frame transmitted by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            dut_ifc->SetErrorState(FaultConfState::ErrPas);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, RtrFlag::Data,
                                                        EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            frm_flags_2 = std::make_unique<FrameFlags>();
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force 7-th data bit to dominant (should be recessive stuff bit), this creates
             *      stuff error.
             *   3. Insert Passive Error frame to monitored frame from next bit. Insert Active
             *      Error frame to driven frame from the same bit.
             *   4. Remove last bit of intermission in both driven and monitored frame.
             *   5. Append next frame after the first frame as if received by DUT!
             *   6. Append the original frame after 2nd frame, because DUT will retransmitt it!
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            drv_bit_frm->InsertActErrFrm(7, BitKind::Data);

            drv_bit_frm->RemoveBit(2, BitKind::Interm);
            mon_bit_frm->RemoveBit(2, BitKind::Interm);

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());

            mon_bit_frm_2->ConvRXFrame();
            /* IUT will resynchronize due to input delay. Compensate it*/
            mon_bit_frm_2->GetBitOf(0, BitKind::Sof)
                ->GetFirstTQIter(BitPhase::Sync)->Lengthen(dut_input_delay);
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            /* Append the original frame, retransmitted by DUT after 2nd frame! */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);
            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();
            CheckRxFrame(*gold_frm_2);

            return FinishElemTest();
        }

};