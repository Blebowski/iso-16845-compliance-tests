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
 * @date 8.8.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.2
 *
 * @brief This test verifies that an IUT acting as a transmitter generates an
 *        overload frame when it detects a dominant bit on the eighth bit of
 *        an error or an overload delimiter.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          FDF = 0
 *      CAN FD Enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 dominant bit on the eighth bit of an error delimiter, error
 *             applied in data field;
 *          #2 dominant bit on the eighth bit of an overload delimiter
 *             following a data frame.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an error frame or overload frame
 *  according to elementary test cases.
 *  Then, the LT forces the eighth bit of the delimiter to a dominant state.
 *
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the dominant bit generated by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_4_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            /* Standard settings for tests where IUT is transmitter */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            /*
             * Set TEC, so that IUT becomes error passive. Keep sufficient
             * reserve from 128 for decrements due to test frames!
             */
            dut_ifc->SetTec(200);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, RtrFlag::Data,
                                                       EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Turn driven frame as received.
             *  2. In first elementary test, force 7-th data bit (should be recessive stuff bit) to
             *     dominant. In second elementary test, force first bit of intermission to dominant.
             *     Insert Error Frame (first elementary test) or Overload frame (second elementary
             *     test) from next bit on monitored frame. Insert passive Error frame also to
             *     driven frame.
             *  3. Force last bit of Error delimiter (first elementary test), Overload delimiter
             *     (second elementary test) to dominant.
             *  4. Insert Overload frame from next bit on monitored frame. Insert Passive Error
             *     frame on driven frame so that LT does not affect IUT.
             *  5. Insert 8-more bits after intermission (behin 2-nd overload frame). This emulates
             *     suspend transmission.
             *  6. In first elemenary test, append the same frame after first frame because frame
             *     shall be retransmitted (due to error frame). This frame should immediately
             *     follow last bit of suspend. In second elementary test, frame shall not be
             *     re-transmitted, because there were only overload frames, so append only dummy
             *     bits to check that unit does not retransmitt (there were only overload frames)!
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            if (elem_test.index_ == 1)
            {
                Bit *data_stuff_bit = drv_bit_frm->GetBitOf(6, BitKind::Data);
                data_stuff_bit->val_ = BitVal::Dominant;
                mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);
                drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            }
            else
            {
                Bit *first_interm_bit = drv_bit_frm->GetBitOf(0, BitKind::Interm);
                drv_bit_frm->FlipBitAndCompensate(first_interm_bit, dut_input_delay);
                mon_bit_frm->InsertOvrlFrm(1, BitKind::Interm);
                drv_bit_frm->InsertPasErrFrm(1, BitKind::Interm);
            }

            Bit *last_delim_bit = drv_bit_frm->GetBitOf(7, BitKind::ErrDelim);
            size_t last_delim_index = drv_bit_frm->GetBitIndex(last_delim_bit);
            drv_bit_frm->FlipBitAndCompensate(last_delim_bit, dut_input_delay);

            mon_bit_frm->InsertOvrlFrm(last_delim_index + 1);
            drv_bit_frm->InsertPasErrFrm(last_delim_index + 1);

            drv_bit_frm->AppendSuspTrans();
            mon_bit_frm->AppendSuspTrans();

            if (elem_test.index_ == 1){
                drv_bit_frm_2->ConvRXFrame();
                drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
                mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());
            } else {
                for (int k = 0; k < 15; k++)
                {
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            this->dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};