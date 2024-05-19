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
 * @date 12.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.5
 *
 * @brief The purpose of this test is to verify the point of time at which a
 *        message transmitted by the IUT is taken to be valid.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled - FDF = 0
 *      CAN FD enabled - FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 *
 *      #1 On the first bit of the intermission field of the frame sent by the
 *         IUT, the LT forces the bit value to dominant.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a data frame. The LT causes the IUT to
 *  generate an overload frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall not restart any frame after the overload frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_1_5 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            /* Basic setup for tests where IUT transmits */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received (insert ACK).
             *   2. Force first bit of driven frame to dominant.
             *   3. Insert overload flag from 2nd bit of intermission further!
             *   4. Insert 15 recessive bits at the end of Overload delimiter.
             *      This checks that DUT does not retransmitt the frame!
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->CompensateEdgeForInputDelay(
                drv_bit_frm->GetBitOf(0, BitKind::Ack), dut_input_delay);

            Bit *first_interm_bit = drv_bit_frm->GetBitOf(0, BitKind::Interm);
            drv_bit_frm->FlipBitAndCompensate(first_interm_bit, dut_input_delay);

            mon_bit_frm->InsertOvrlFrm(1, BitKind::Interm);

            Bit *overload_end_bit = mon_bit_frm->GetBitOf(6, BitKind::OvrlDelim);
            size_t bit_index = mon_bit_frm->GetBitIndex(overload_end_bit);
            for (size_t i = 0; i < 15; i++)
                mon_bit_frm->InsertBit(BitKind::OvrlDelim, BitVal::Recessive, bit_index);

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