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
 * @date 14.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.3.2
 *
 * @brief The purpose of this test is to verify that an IUT accepts a frame
 *        starting after the second bit of the intermission following the error
 *        frame it has transmitted.
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
 *      #1 Frame is started 2 bits after the end of the error delimiter.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field.
 *  The LT sends a valid frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall acknowledge the test frame in data field.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_3_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_,
                            RtrFlag::DataFrame);
            golden_frm = std::make_unique<Frame>(*frame_flags, 1, &error_data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
             *      This will cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Remove last bit of Intermission (after error frame)
             *   5. Insert second frame directly after first frame.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            driver_bit_frm->RemoveBit(2, BitType::Intermission);
            monitor_bit_frm->RemoveBit(2, BitType::Intermission);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /* Generate frame 2 - randomize everything */
            frame_flags_2 = std::make_unique<FrameFlags>();
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2);
            RandomizeAndPrint(golden_frm_2.get());

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            monitor_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm_2);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};