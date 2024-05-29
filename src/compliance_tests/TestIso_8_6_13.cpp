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
 * @date 25.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.13
 *
 * @brief This test verifies that an IUT acting as a transmitter does not
 *        change the value of its TEC when receiving a 13-bit long overload
 *        flag.
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
 *     #1 LT sends a sequence of 1 (to cause an overload flag) + 13 (test
 *        pattern) dominant bits.
 *
 * Setup:
 *  The LT forces the IUT to increase its TEC.
 *  The LT causes the IUT to transmit a frame, where the LT causes an error
 *  scenario to set TEC to 08 h before the test started.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  After the last bit of the EOF, the LT sends a sequence of dominant bits
 *  according to elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value decreased by 1.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_13 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            dut_ifc->SetTec(8);
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
             *   1. Turn driven frame as if received.
             *   2. Force first bit of Interframe to dominant (overload flag)
             *   3. Insert Expected overload frame.
             *   4. Insert 7 dominant bits after the overload flag to driven frame, insert 7
             *      recessive bits to monitored frame.
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            drv_bit_frm->FlipBitAndCompensate(
                drv_bit_frm->GetBitOf(0, BitKind::Interm), dut_input_delay);

            drv_bit_frm->InsertOvrlFrm(1, BitKind::Interm);
            mon_bit_frm->InsertOvrlFrm(1, BitKind::Interm);

            for (size_t i = 0; i < 7; i++)
            {
                size_t bit_index = drv_bit_frm->GetBitIndex(
                                        drv_bit_frm->GetBitOf(0, BitKind::OvrlDelim));
                drv_bit_frm->InsertBit(BitKind::OvrlFlag, BitVal::Dominant, bit_index);
                mon_bit_frm->InsertBit(BitKind::OvrlDelim, BitVal::Recessive, bit_index);
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();
            CheckTecChange(tec_old, -1);

            return FinishElemTest();
        }

};