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
 * @date 21.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.10
 *
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting a form error during the transmission of an
 *        overload delimiter.
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
 *     #1 corrupting the second bit of the overload delimiter;
 *     #2 corrupting the seventh bit of the overload delimiter;
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame after a data
 *  frame.
 *  The LT corrupts one of the recessive bits of the overload delimiter
 *  according to elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be increased by 8 at the corrupted bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_10 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Flip first bit of intermission to dominant (Overload condition)
             *   3. Insert Overload frame from next bit on to monitored frame. Insert Passive Error
             *      frame to driven frame (TX/RX feedback enabled)
             *   4. Flip 2 or 7-th bit of overload delimiter to dominant.
             *   5. Insert next Error frame from next bit on.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->FlipBitAndCompensate(
                driver_bit_frm->GetBitOf(0, BitType::Intermission), dut_input_delay);

            driver_bit_frm->InsertPassiveErrorFrame(1, BitType::Intermission);
            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

            int bit_to_flip;
            if (elem_test.index_ == 1)
                bit_to_flip = 1;
            else
                bit_to_flip = 6;
            int bit_index = driver_bit_frm->GetBitIndex(
                driver_bit_frm->GetBitOf(bit_to_flip, BitType::ErrorDelimiter));

            driver_bit_frm->FlipBitAndCompensate(driver_bit_frm->GetBit(bit_index), dut_input_delay);

            driver_bit_frm->InsertPassiveErrorFrame(bit_index + 1);
            monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

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

            // +8 for error, -1 for retransmission. In firt elem test, TEC is 0, no retransmission!
            if (test_variant == TestVariant::Common && elem_test.index_ == 1)
                CheckTecChange(tec_old, 8);
            else
                CheckTecChange(tec_old, 7);

            return FinishElementaryTest();
        }

};