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
 * @date 27.3.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.1
 *
 * @brief This test verifies the behaviour of the IUT when receiving a
 * correct data frame with different identifiers and different numbers of data
 * bytes in base format frame.
 *
 * @version CAN FD Enabled, CAN FD Tolerant, Classical CAN
 *
 * Test variables:
 *  ID
 *  DLC
 *  FDF = 0
 *
 * Elementary test cases:
 *  CAN FD Enabled, CAN FD Tolerant, Classical CAN :
 *      The CAN ID will be element of: ∈ [000 h , 7FF h]
 *          Different CAN IDs are used for test.
 *          Elementary test cases:
 *              #1 CAN ID = 555 h
 *              #2 CAN ID = 2AA h
 *              #3 CAN ID = 000 h
 *              #4 CAN ID = 7FF h
 *              #5 CAN ID = a random value
 *      Tested number of data bytes: ∈ [0, 8]
 *      Number of tests: 45
 *
 *  CAN FD Enabled :
 *      The CAN ID will be element of: ∈ [000 h , 7FF h]
 *          Different CAN IDs are used for test.
 *          Elementary test cases:
 *              #1 CAN ID = 555 h
 *              #2 CAN ID = 2AA h
 *              #3 CAN ID = 000 h
 *              #4 CAN ID = 7FF h
 *              #5 CAN ID = a random value
 *      Tested number of data bytes:
 *          ∈ [0, 8] ∪[12] ∪ [16] ∪ [20] ∪ [24] ∪ [32] ∪ [48] ∪ [64]
 *      Number of tests: 80
 *
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The test system sends a frame with ID and DLC as specified in elementary
 *  test cases definition.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state should match the data
 *  sent in the test frame.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 45; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameKind::Can20));
            for (int i = 0; i < 80; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc = (elem_test.index_ - 1) / 5;
            int can_id;

            switch (elem_test.index_)
            {
                case 1:
                    can_id = 0x555;
                    break;
                case 2:
                    can_id = 0x2AA;
                    break;
                case 3:
                    can_id = 0x000;
                    break;
                case 4:
                    can_id = 0x7FF;
                    break;
                case 5:
                    can_id = rand() % (int)pow(2, 11);
                    break;
                default:
                    can_id = 0x0;
                    break;
            }

            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentKind::Base,
                                                       RtrFlag::Data);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlc, can_id);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitored frame as if received.
             *************************************************************************************/
            monitor_bit_frm->ConvRXFrame();

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};
