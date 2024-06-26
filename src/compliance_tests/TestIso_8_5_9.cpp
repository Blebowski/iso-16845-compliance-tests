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
 * @test ISO16845 8.5.9
 *
 * @brief The purpose of this test is to verify that a passive state IUT does
 *        not transmit a frame starting with an identifier and without
 *        transmitting SOF when detecting a dominant bit on the third bit of
 *        the intermission field.
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
 *      #1 dominant bit on the third bit of the intermission field;
 *      #2 dominant bit on the first bit of Suspend transmission;
 *      #3 dominant bit on the seventh bit of Suspend transmission.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT corrupts this frame and corrupts the following error frame to set
 *  the IUT into passive state.
 *  The error frame shall end with error delimiter followed by the inter-
 *  mission field.
 *  At the bit, as specified in elementary test cases after the error delimiter,
 *  the LT starts sending a frame with lower priority
 *
 * Response:
 *  The IUT shall not start the re-transmission before the end of the frame
 *  sent by the LT.
 *  The IUT shall acknowledge the frame sent by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_9 : public test::TestBase
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
            uint8_t data_byte = 0x80;

            /* First frame */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* Second frame */
            frm_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_kind_);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force 7-th data bit to dominant to cause stuff error.
             *   3. Insert Passive Error frame from next bit to monitored frame. Insert Passive
             *      Error frame to driven frame.
             *   4. Modify frames according to elementary test cases:
             *       First elementary test:
             *         Remove last bit of Intermission in both driven and monitored frame.
             *       Second elementary test:
             *         Do nothing, appending after third bit of intermission corresponds
             *         to detecting dominant by IUT during first bit of its suspend field.
             *       Third elementary test:
             *         Append 6 bits of Suspend transmission. If then frame is appended in
             *         next step, this corresponds to detecting dominant at 7-th bit of
             *         suspend field!
             *   5. Append next frame. On monitor as is received by IUT, on driver as if sent by
             *      LT. This checks that IUT becomes receiver of this frame!
             *   6. Append the same frame as first frame once again (after this second frame).
             *      This checks that DUT re-transmitts the first frame which had error on 7-th data
             *      bit. Frame is as if received on driven frame.
             *   7. Append one more intermission, since DUT will succesfully retransmitt the frame
             *      and therefore go to suspend! This is needed to separate it from next time step!
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            switch (elem_test.index_)
            {
                case 1:
                    drv_bit_frm->RemoveBit(drv_bit_frm->GetBitOf(2, BitKind::Interm));
                    mon_bit_frm->RemoveBit(mon_bit_frm->GetBitOf(2, BitKind::Interm));
                    break;
                case 3:
                    for (size_t i = 0; i < 7; i++)
                    {
                        drv_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
                        mon_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
                    }
                    break;
                default:
                    break;
            }

            /* Append second one */
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm_2->ConvRXFrame();
            // Compensate IUTs input delay
            mon_bit_frm_2->GetBitOf(0, BitKind::Sof)
                ->GetFirstTQIter(BitPhase::Sync)->Lengthen(dut_input_delay);
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            /* Append third one */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);
            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            /* Append Suspend transmission */
            drv_bit_frm->AppendSuspTrans();
            mon_bit_frm->AppendSuspTrans();

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            dut_ifc->SetErrorState(FaultConfState::ErrPas);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();
            CheckRxFrame(*gold_frm_2);

            return FinishElemTest();
        }

};