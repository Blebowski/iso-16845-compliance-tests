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
 * @date 20.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.21
 *
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when transmitting a frame successfully.
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
 *      #1 The higher prior frame is disturbed by an error to increase REC.
 *
 * Setup:
 *  No action required, the IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *
 *  The LT sends a frame with higher ID priority to cause the IUT to lose
 *  arbitration according to elementary test cases. The IUT will repeat its
 *  transmission after error treatment.
 *
 * Response:
 *  The IUT’s REC value shall be incremented and not decremented after
 *  transmission.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_21 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            SetupMonitorTxTests();

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /*
             * Dont shift bit-rate needed since Transmitted frame after received frame is not
             * handled well with Bit rate shifts due to small resynchronizations in reciver!
             */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                    RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, 0xAB, &error_data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* Second frame the same due to retransmission. */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip driven frame to dominant one one bit before end of Base ID. This bit
             *      should be last recessive bit of Base ID.
             *   2. Loose arbitration on monitored frame from the same bit
             *   3. Flip 7-th bit of data field to dominant. This causes stuff error.
             *   4. Insert Active Error frame from next bit on to driven frame. Insert Passive
             *      Error frame to monitored frame.
             *************************************************************************************/
            Bit *loosing_bit = drv_bit_frm->GetBitOf(9, BitKind::BaseIdent);
            loosing_bit->val_ = BitVal::Dominant;
            int bit_index = drv_bit_frm->GetBitIndex(loosing_bit);

            // Compensate IUTs input delay - lenghten IUTs monitored bit by its input delay,
            // since IUT will re-synchronize due to this delay on the same bit on which it loses
            // arbitration!
            mon_bit_frm->LooseArbit(bit_index);
            mon_bit_frm->GetBit(bit_index + 1)->GetLastTQIter(BitPhase::Sync)
                ->Lengthen(dut_input_delay);

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);

            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(20);
            tec_old = dut_ifc->GetTec();
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();
            CheckRecChange(rec_old, +1);
            CheckTecChange(tec_old, +0);

            FreeTestObjects();
            return FinishElemTest();
        }
};