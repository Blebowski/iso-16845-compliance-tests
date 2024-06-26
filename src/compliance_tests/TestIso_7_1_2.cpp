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
 * @date 10.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.2
 *
 * @brief This test verifies the behaviour of the IUT when receiving a correct
 *        data frame with different identifiers and different numbers of data
 *        bytes in extended format frame.
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
 *      The CAN ID shall be element of: ∈ [00000000 h , 1FFFFFFF h ]
 *          Different CAN IDs are used for test.
 *          Elementary test cases:
 *              #1 CAN ID = 15555555 h
 *              #2 CAN ID = 0AAAAAAA h
 *              #3 CAN ID = 00000000 h
 *              #4 CAN ID = 1FFFFFFF h
 *              #5 CAN ID = random value
 *      Tested number of data bytes: ∈ [0, 8]
 *      Number of tests: 45
 *
 *  CAN FD Enabled :
 *      The CAN ID shall be element of: ∈ [00000000 h , 1FFFFFFF h ]
 *          Different CAN IDs are used for test.
 *          Elementary test cases:
 *              #1 CAN ID = 15555555 h
 *              #2 CAN ID = 0AAAAAAA h
 *              #3 CAN ID = 00000000 h
 *              #4 CAN ID = 1FFFFFFF h
 *              #5 CAN ID = random value
 *      Tested number of data bytes:
 *          ∈ [0, 8] ∪[12] ∪ [16] ∪ [20] ∪ [24] ∪ [32] ∪ [48] ∪ [64]
 *      Number of tests: 80
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
 * Note:
 *  An implementation with limited ID range may not be able to receive the
 *  frame.
 *  An implementation with limited payload capabilities will be tested in range
 *  of their payload capabilities.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 45; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
            for (size_t i = 0; i < 80; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int can_id;
            switch (elem_test.index_)
            {
                case 1:
                    can_id = 0x15555555;
                    break;
                case 2:
                    can_id = 0x0AAAAAAA;
                    break;
                case 3:
                    can_id = 0x00000000;
                    break;
                case 4:
                    can_id = 0x1FFFFFFF;
                    break;
                case 5:
                    can_id = rand() % CAN_EXTENDED_ID_MAX;
                    break;
                default:
                    can_id = 0x0;
                    break;
            }

            uint8_t dlc = static_cast<uint8_t>((elem_test.index_ - 1) / 5);

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                            IdentKind::Ext, RtrFlag::Data);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, can_id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitored frame as if received.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            FreeTestObjects();
            return FinishElemTest();
        }

};