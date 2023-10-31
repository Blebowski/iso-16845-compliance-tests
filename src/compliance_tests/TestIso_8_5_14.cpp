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
 * @date 14.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.14
 *
 * @brief The purpose of this test is to verify that the recovery time of an
 *        error passive IUT detecting an error is at most 31 bit times.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          FDF = 0
 *      CAN FD enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform:
 *          #1 At the bit position following the end of the passive error flag,
 *             the LT starts to send 6 dominant bits.
 *
 * Setup:
 *  The IUT is set to the TEC passive state
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT corrupts a bit in data field of this frame causing the IUT to
 *  generate a passive error flag according to elementary test cases.
 *
 * Response:
 *  The IUT shall re-transmit the same frame 31 bit times after the detection
 *  of the corrupted bit.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_14 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            /* Basic settings where IUT is transmitter */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            /* To be error passive */
            dut_ifc->SetTec(160);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* Second frame the same due to retransmission. */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force 7-th data bit to dominant to cause stuff error.
             *   2. Insert Passive Error frame to both driven and monitored frames from next bit
             *      on.
             *   3. Insert 6 dominant bits from first position of error delimiter to driven frame.
             *      Insert 6 recessive bits to monitored frame.
             *   4. Append suspend transmission (both driven and monitored frames).
             *   5. Append retransmitted frame as if transmitted by IUT.
             *
             * Note: After the corrupted bit, there will be:
             *    6 bits            passive error frame
             *    6 bits            waiting for recessive bit to start error delimiter
             *    8 bit             error delimiter
             *    3 intermission    intermission
             *    8 suspend         suspend since IUT is error passive
             * Together, this is 31 bits as described in test description!
             *************************************************************************************/
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            Bit *err_delim = drv_bit_frm->GetBitOf(0, BitKind::ErrDelim);
            int bit_index = drv_bit_frm->GetBitIndex(err_delim);
            for (int i = 0; i < 6; i++)
            {
                drv_bit_frm->InsertBit(BitKind::PasErrFlag, BitVal::Dominant, bit_index);
                mon_bit_frm->InsertBit(BitKind::PasErrFlag, BitVal::Recessive, bit_index);
            }

            // Compensate IUTs input delay which would cause positive resynchronisation.
            // 7-th bit of Passive error Flag in driven frame is first bit of second error flag
            drv_bit_frm->CompensateEdgeForInputDelay(
                drv_bit_frm->GetBitOf(6, BitKind::PasErrFlag), dut_input_delay);

            drv_bit_frm->AppendSuspTrans();
            mon_bit_frm->AppendSuspTrans();

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
            this->dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};