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
 * @date 05.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.2
 *
 * @brief This test verifies the capacity of the IUT to transmit a data frame
 *        with different identifiers and different numbers of data in an
 *        extended format frame.
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          ID
 *          DLC
 *          FDF = 0
 *
 *  CAN FD Enabled:
 *          ID,
 *          DLC,
 *          FDF = 1, res = 0, BRS = 1, ESI = 0
 *      A device with limited payload bytes will be tested with the CC h
 *      padding payload for the not supported bytes of payload.
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      The CAN ID shall be element of:
 *          ∈ [000 h , 7FF h ]
 *      Different CAN IDs are used for test.
 *          #1 CAN ID = 15555555 h
 *          #2 CAN ID = 0AAAAAAA h
 *          #3 CAN ID = 00000000 h
 *          #4 CAN ID = 1FFFFFFF h
 *          #5 CAN ID = a random value
 *      Tested number of data bytes:∈ [0, 8].
 *      Number of tests: 9 × selected ID
 *
 *  CAN FD Enabled:
  *      The CAN ID shall be element of:
 *          ∈ [000 h , 7FF h ]
 *      Different CAN IDs are used for test.
 *          #1 CAN ID = 15555555 h
 *          #2 CAN ID = 0AAAAAAA h
 *          #3 CAN ID = 00000000 h
 *          #4 CAN ID = 1FFFFFFF h
 *          #5 CAN ID = a random value
 *      Tested number of data bytes:∈ [0, 8].
 *      Number of tests: 16 × selected ID
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for each elementary test. The LT causes the
 *  IUT to transmit a data frame with the parameters according to elementary
 *  test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The content of the frame shall match the LT request.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_1_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 45; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
            for (size_t i = 0; i < 80; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int id;
            uint8_t dlc = static_cast<uint8_t>((elem_test.index_ - 1) / 5);

            switch ((elem_test.index_ - 1) % 5)
            {
                case 0:
                    id = 0x15555555;
                    break;
                case 1:
                    id = 0x0AAAAAAA;
                    break;
                case 2:
                    id = 0x00000000;
                    break;
                case 3:
                    id = 0x1FFFFFFF;
                    break;
                case 4:
                    id = rand() % ((int)pow(2, 29));
                    break;
                default:
                    break;
            }

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Ext,
                                RtrFlag::Data, BrsFlag::DoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received (insert ACK).
             *************************************************************************************/
            drv_bit_frm->ConvRXFrame();

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