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
 * @test ISO16845 8.6.16
 *
 * @brief This test verifies that an IUT acting as a transmitter does not
 *        change the value of its TEC when detecting a form error on the last
 *        bit of the error delimiter it is transmitting.
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
 *     #1 LT sends 1 dominant bit.
 *
 * Setup:
 *  The LT forces the IUT to increase its TEC.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame and causes the IUT to send an
 *  active error flag in data filed.
 *  After the last bit of the error flag, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be 8.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_16 : public test::TestBase
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
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Corrupt 7-th bit of data field. This should be recessive stuff bit. Force it to
             *      dominant.
             *   3. Insert Active Error frame to monitored frame from next bit on. Insert Passive
             *      Error frame to driven frame from next bit on.
             *   4. Flip last bit of Error delimiter (8-th) to dominant in driven frame.
             *   5. Insert Overload frame from next bit on. Insert Passive error frame from next
             *      bit on.
             *   6. Append retransmitted frame.
             *************************************************************************************/
            driver_bit_frm->ConvRXFrame();

            driver_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            driver_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            monitor_bit_frm->InsertActErrFrm(7, BitKind::Data);

            driver_bit_frm->FlipBitAndCompensate(
                driver_bit_frm->GetBitOf(7, BitKind::ErrDelim), dut_input_delay);

            driver_bit_frm->InsertPasErrFrm(0, BitKind::Interm);
            monitor_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

            driver_bit_frm_2->ConvRXFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            /* +1 for original error frame, -1 for succesfull retransmission */
            CheckTecChange(tec_old, +7);

            return FinishElementaryTest();
        }

};