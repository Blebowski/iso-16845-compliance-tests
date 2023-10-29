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
 * @date 26.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.17
 *
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when receiving a 13-bit length overload flag.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 *
 *  CAN FD Enabled
 *      REC, FDF = 1
 *
 * Elementary test cases:
 *      #1 7 dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  a) The test system causes a receive error to initialize the REC value to 9.
 *  b) The LT causes the IUT to generate an overload frame after a valid frame
 *     reception (REC-1).
 *     After the overload flag sent by the IUT, the LT sends a sequence
 *     according to elementary test cases.
 *
 * Response:
 *  The correct frame up to the EOF will decrement REC and the overload
 *  enlargement will not increase REC.
 *  The IUT’s REC value shall be 8.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_17 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force last bit of EOF to DOMINANT.
             *   3. Insert expected overload frame from first bit of Intermission.
             *   4. Insert 7 Dominant bits to driver on can_tx and 7 Recessive bits to monitor on
             *      can_rx from first bit of overload delimiter.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Eof)->val_ = BitVal::Dominant;

            mon_bit_frm->InsertOvrlFrm(0, BitKind::Interm);
            drv_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

            Bit *overload_delim = drv_bit_frm->GetBitOf(0, BitKind::OvrlDelim);
            int bit_index = drv_bit_frm->GetBitIndex(overload_delim);

            for (int i = 0; i < 7; i++)
            {
                drv_bit_frm->InsertBit(BitKind::OvrlFlag, BitVal::Dominant, bit_index);
                mon_bit_frm->InsertBit(BitKind::OvrlFlag, BitVal::Recessive, bit_index);
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(9);
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);

            CheckLTResult();
            CheckRxFrame(*gold_frm);
            /* Only for sucesfull frame reception */
            CheckRecChange(rec_old, -1);

            FreeTestObjects();
            return FinishElemTest();
        }
};