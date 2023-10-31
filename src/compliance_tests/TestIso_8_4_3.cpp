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
 * @date 9.8.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.3
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a data frame starting with the identifier and without transmitting
 *        SOF, when detecting a dominant bit on the third bit of the intermi-
 *        ssion field following an overload frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 0
 *      CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 *
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits.
 *          #2 the identifier shall start with 5 recessive bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT disturbs the transmitted frame with an error frame, then the LT causes
 *  the IUT to generate an overload frame immediately after the error frame.
 *  Then, the LT forces the third bit of the intermission following the overload
 *  delimiter to dominant state.
 *
 * Response:
 *  The IUT shall repeat the frame starting with the identifier without
 *  transmitting any SOF.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_4_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (int i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            /* Standard settings for tests where IUT is transmitter */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            int ids[] = {
                0x7B,
                0x3B
            };

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                    RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, ids[elem_test.index_ - 1],
                                                 &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Turn driven frame as received.
             *  2. Force 7-th data bit to Dominant. This should be recessive stuff bit. Insert
             *     Active Error frame from next bit on, to monitored frame. Insert Passive Error
             *     frame to driven frame.
             *  3. Force 8-th bit of Error delimiter to dominant. Insert Overload frame from next
             *     bit on to monitored frame. Insert Passive Error frame to driven frame.
             *  4. Force third bit of intermission after overload frame to dominant (in driven
             *     frame).
             *  5. Remove SOF bit from retransmitted frame. Append retransmitted frame behind the
             *     first frame. Second driven frame is turned received.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);
            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            Bit *last_err_delim_bit = drv_bit_frm->GetBitOf(7, BitKind::ErrDelim);
            drv_bit_frm->FlipBitAndCompensate(last_err_delim_bit, dut_input_delay);

            int last_err_delim_index = drv_bit_frm->GetBitIndex(last_err_delim_bit);
            mon_bit_frm->InsertOvrlFrm(last_err_delim_index + 1);
            drv_bit_frm->InsertPasErrFrm(last_err_delim_index + 1);

            Bit *third_intermission_bit = drv_bit_frm->GetBitOf(2, BitKind::Interm);
            drv_bit_frm->FlipBitAndCompensate(third_intermission_bit, dut_input_delay);

            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm_2->RemoveBit(drv_bit_frm_2->GetBitOf(0, BitKind::Sof));
            mon_bit_frm_2->RemoveBit(mon_bit_frm_2->GetBitOf(0, BitKind::Sof));

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            this->dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};