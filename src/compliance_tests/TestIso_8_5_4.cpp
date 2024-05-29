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
 * @test ISO16845 8.5.4
 *
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter is able to receive a frame during the suspend
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
 *   Elementary tests to perform:
 *      #1 the received frame starts on the first bit of the suspend transmission;
 *      #2 the received frame starts on the fourth bit of the suspend transmission;
 *      #3 the received frame starts on the eighth bit of the suspend transmission.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  At the end of the EOF and intermission fields, the LT sends a frame according to
 *  elementary test-cases.
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

class TestIso_8_5_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1 , FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            dut_ifc->SetErrorState(FaultConfState::ErrPas);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* ESI needed for CAN FD variant */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                                                       EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            frm_flags_2 = std::make_unique<FrameFlags>();
            gold_frm_2 = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Insert suspend field to both driven and monitored frames (it is not inserted by
             *      default frame construction). Insert only necessary portion of field to emulate
             *      start of next frame in the middle of it!
             *   3. Prolong SOF of second monitored frame to account for input delay of IUT.
             *   4. Append next frame to driven frame. Append next frame as if received to
             *      monitored frame.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            size_t num_suspend_bits = 0;
            switch (elem_test.index_)
            {
                case 1:
                    num_suspend_bits = 0;
                    break;
                case 2:
                    num_suspend_bits = 3;
                    break;
                case 3:
                    num_suspend_bits = 7;
                    break;
                default:
                    break;
            }

            for (size_t i = 0; i < num_suspend_bits; i++)
            {
                drv_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
            }

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm_2->ConvRXFrame();
            mon_bit_frm_2->GetBitOf(0, BitKind::Sof)
                ->GetFirstTQIter(BitPhase::Sync)->Lengthen(dut_input_delay);
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