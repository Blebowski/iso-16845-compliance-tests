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
 * @date 15.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.21
 *
 * @brief This test verifies that the IUT does not change the value of its TEC
 *        when receiving a frame with error in it after arbitration lost.
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
 *     #1 The high priority frame is disturbed by an error to increase REC.
 *
 * Setup:
 *  No action required. The IUT is left in the default state.
 *  The LT causes the IUT to transmit a frame, where the LT causes an error
 *  scenario to init TEC to 08 h before test started.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT sends a frame with higher ID priority to cause the IUT to lose
 *  arbitration according to elementary test cases.
 *  The LT receives the repeated frame without error.
 *
 * Response:
 *  The IUT’s TEC value shall be unchanged equal to setup value.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_21 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameKind::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            dut_ifc->SetTec(8);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            /* Sent by LT */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentKind::Base,
                                RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x50, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            /* Sent by IUT */
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentKind::Base,
                                RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, 0x1, 0x51, &data_byte);
            RandomizeAndPrint(golden_frm_2.get());

            /* Since IUT will loose arbitration, do both driven and monitored frames as the ones
             * from IUT, correct the last bit later
             */
            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* In retransmitted frame, there will be no arbitration lost */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip last bit of base id of monitored frame to recessive since IUT actually
             *      sends ID ending with 1.
             *   2. Loose arbitration in monitored frame on last bit of Base id.
             *   3. Flip 7-th bit of data field to dominant. This should cauase stuff error.
             *   4. Insert active error frame to monitored frame from next bit. Insert passive
             *      error frame to driven frame (TX/RX feedback enabled).
             *   5. Append retransmitted frame by IUT.
             *************************************************************************************/
            Bit *last_base_id = monitor_bit_frm->GetBitOfNoStuffBits(10, BitKind::BaseIdent);
            last_base_id->val_ = BitVal::Dominant;

            monitor_bit_frm->LooseArbit(last_base_id);

            // Compensate IUTs input delay, since it will resynchronize due to bits which are
            // further sent by LT.
            last_base_id->GetLastTQIter(BitPhase::Ph2)->Lengthen(dut_input_delay);

            driver_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            driver_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            monitor_bit_frm->InsertActErrFrm(7, BitKind::Data);

            driver_bit_frm_2->ConvRXFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm_2.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            /* TODO: ISO says there should be 0 change here! But due to retransmission,
             *       there shall be -1 here IMHO! This is ISO error and shall be reported!
             */
            CheckTecChange(tec_old, -1);
            CheckRecChange(rec_old, +1);

            return FinishElementaryTest();
        }

};