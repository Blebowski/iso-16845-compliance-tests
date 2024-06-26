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
 * @date 24.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.8
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a frame on reception of an SOF starting at the third bit of the
 *        intermission field following the arbitration winning frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          Intermission field 2 bits, FDF = 0
 *      CAN FD enabled:
 *          Intermission field 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits;
 *          #2 the identifier shall start with 5 recessive bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT sends a frame with higher priority at the same time, to force an
 *  arbitration lost for the frame sent by the IUT. At start of intermission,
 *  the LT waits for 2 bit times before sending an SOF.
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

class TestIso_8_1_8 : public test::TestBase
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

            /* Basic setup for tests where IUT transmits */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base);

            int gold_ids[] = {
                0x7B,
                0x3B
            };
            int lt_ids[] = {
                0x7A,
                0x3A
            };

            gold_frm = std::make_unique<Frame>(*frm_flags, 0x0, gold_ids[elem_test.index_ - 1]);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags, 0x0, lt_ids[elem_test.index_ - 1]);

            drv_bit_frm = ConvBitFrame(*gold_frm_2);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Loose arbitration on last bit of ID.
             *   2. Force last bit of driven frame intermission to dominant. This emulates LT
             *      sending SOF after 2 bits of intermission.
             *   3. Append the same frame to driven and monitored frame. On driven frame, turn
             *      second frame as if received.
             *   4. Remove SOF from 2nd monitored frame.
             *************************************************************************************/
            Bit *last_id_bit = mon_bit_frm->GetBitOfNoStuffBits(10, BitKind::BaseIdent);
            mon_bit_frm->LooseArbit(last_id_bit);

            /* Compensate input delay, lenghten bit on which arbitration was lost */
            last_id_bit->GetLastTQIter(BitPhase::Ph2)->Lengthen(dut_input_delay);

            drv_bit_frm->GetBitOf(2, BitKind::Interm)->val_ = BitVal::Dominant;

            drv_bit_frm_2->ConvRXFrame();

            mon_bit_frm_2->RemoveBit(0, BitKind::Sof);
            drv_bit_frm_2->RemoveBit(0, BitKind::Sof);

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

            FreeTestObjects();
            return FinishElemTest();
        }

};