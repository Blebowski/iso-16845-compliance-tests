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
 * @date 3.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.5
 *
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid
 *        extended format frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN   : SRR, FDF, r0
 *  CAN FD Tolerant : SRR, FDF, r0=0
 *  CAN FD Enabled  : SRR, RRS, FDF=1
 *
 * Elementary test cases:
 *  Classical CAN:
 *      TEST    SRR     r0      FDF
 *       #1      1       1       1
 *       #2      1       1       0
 *       #3      1       0       1
 *       #4      0       1       1
 *       #5      0       1       0
 *       #6      0       0       1
 *       #7      0       0       0
 *
 *  CAN FD Tolerant:
 *      TEST    SRR     r0
 *       #1      1       1
 *       #2      0       1
 *       #3      0       0
 *
 *  CAN FD Enabled:
 *      TEST    SRR     RRS
 *       #1      1       1
 *       #2      0       1
 *       #3      0       0
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_5 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::OneToOne);

            int num_elem_tests;
            FrameKind frame_type;
            if (test_variants[0] == TestVariant::Can20)
            {
                num_elem_tests = 7;
                frame_type = FrameKind::Can20;
            }
            else if (test_variants[0] == TestVariant::CanFdTol)
            {
                num_elem_tests = 3;
                frame_type = FrameKind::Can20;
            }
            else
            {
                num_elem_tests = 3;
                frame_type = FrameKind::CanFd;
            }

            for (int i = 0; i < num_elem_tests; i++)
                AddElemTest(test_variants[0], ElemTest(i + 1, frame_type));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data = 0xAA;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                            IdentKind::Ext, RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, 0x55, &data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force bits per Test variant or elementary test.
             *   2. Update frames (needed since CRC might have changed).
             *   3. Turned monitored frame received.
             *************************************************************************************/

            if (test_variant == TestVariant::Can20)
            {
                Bit *srr_driver = drv_bit_frm->GetBitOf(0, BitKind::Srr);
                Bit *srr_monitor = mon_bit_frm->GetBitOf(0, BitKind::Srr);
                Bit *r0_driver = drv_bit_frm->GetBitOf(0, BitKind::R0);
                Bit *r0_monitor = mon_bit_frm->GetBitOf(0, BitKind::R0);

                /* In CAN 2.0 Extended frame format, R1 is at position of FDF */
                Bit *r1_driver = drv_bit_frm->GetBitOf(0, BitKind::R1);
                Bit *r1_monitor = mon_bit_frm->GetBitOf(0, BitKind::R1);

                switch (elem_test.index_)
                {
                case 1:
                    srr_driver->val_ = BitVal::Recessive;
                    srr_monitor->val_ = BitVal::Recessive;
                    r0_driver->val_ = BitVal::Recessive;
                    r0_monitor->val_ = BitVal::Recessive;
                    r1_driver->val_ = BitVal::Recessive;
                    r1_monitor->val_ = BitVal::Recessive;
                    break;
                case 2:
                    srr_driver->val_ = BitVal::Recessive;
                    srr_monitor->val_ = BitVal::Recessive;
                    r0_driver->val_ = BitVal::Recessive;
                    r0_monitor->val_ = BitVal::Recessive;
                    r1_driver->val_ = BitVal::Dominant;
                    r1_monitor->val_ = BitVal::Dominant;
                    break;
                case 3:
                    srr_driver->val_ = BitVal::Recessive;
                    srr_monitor->val_ = BitVal::Recessive;
                    r0_driver->val_ = BitVal::Dominant;
                    r0_monitor->val_ = BitVal::Dominant;
                    r1_driver->val_ = BitVal::Recessive;
                    r1_monitor->val_ = BitVal::Recessive;
                    break;
                case 4:
                    srr_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r0_driver->val_ = BitVal::Recessive;
                    r0_monitor->val_ = BitVal::Recessive;
                    r1_driver->val_ = BitVal::Recessive;
                    r1_monitor->val_ = BitVal::Recessive;
                    break;
                case 5:
                    srr_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r0_driver->val_ = BitVal::Recessive;
                    r0_monitor->val_ = BitVal::Recessive;
                    r1_driver->val_ = BitVal::Dominant;
                    r1_monitor->val_ = BitVal::Dominant;
                    break;
                case 6:
                    srr_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r0_driver->val_ = BitVal::Dominant;
                    r0_monitor->val_ = BitVal::Dominant;
                    r1_driver->val_ = BitVal::Recessive;
                    r1_monitor->val_ = BitVal::Recessive;
                    break;
                case 7:
                    srr_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r0_driver->val_ = BitVal::Dominant;
                    r0_monitor->val_ = BitVal::Dominant;
                    r1_driver->val_ = BitVal::Dominant;
                    r1_monitor->val_ = BitVal::Dominant;
                default:
                    break;
                }

            } else if (test_variant == TestVariant::CanFdTol) {
                Bit *srr_driver = drv_bit_frm->GetBitOf(0, BitKind::Srr);
                Bit *srr_monitor = mon_bit_frm->GetBitOf(0, BitKind::Srr);
                Bit *r0_driver = drv_bit_frm->GetBitOf(0, BitKind::R0);
                Bit *r0_monitor = mon_bit_frm->GetBitOf(0, BitKind::R0);

                switch (elem_test.index_)
                {
                case 1:
                    srr_driver->val_ = BitVal::Recessive;
                    srr_monitor->val_ = BitVal::Recessive;
                    r0_driver->val_ = BitVal::Recessive;
                    r0_monitor->val_ = BitVal::Recessive;
                    break;
                case 2:
                    srr_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r0_driver->val_ = BitVal::Recessive;
                    r0_monitor->val_ = BitVal::Recessive;
                    break;
                case 3:
                    srr_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r0_driver->val_ = BitVal::Dominant;
                    r0_monitor->val_ = BitVal::Dominant;
                    break;
                default:
                    break;
                }

            } else if (test_variant == TestVariant::CanFdEna) {
                Bit *srr_driver = drv_bit_frm->GetBitOf(0, BitKind::Srr);
                Bit *srr_monitor = mon_bit_frm->GetBitOf(0, BitKind::Srr);
                /* R1 bit in CAN FD frames is RRS bit position */
                Bit *r1_driver = drv_bit_frm->GetBitOf(0, BitKind::R1);
                Bit *r1_monitor = mon_bit_frm->GetBitOf(0, BitKind::R1);
                switch (elem_test.index_)
                {
                case 1:
                    srr_driver->val_ = BitVal::Recessive;
                    r1_driver->val_ = BitVal::Recessive;
                    srr_monitor->val_ = BitVal::Recessive;
                    srr_driver->val_ = BitVal::Recessive;
                    break;
                case 2:
                    srr_driver->val_ = BitVal::Dominant;
                    r1_driver->val_ = BitVal::Recessive;
                    srr_monitor->val_ = BitVal::Dominant;
                    r1_monitor->val_ = BitVal::Recessive;
                    break;
                case 3:
                    srr_driver->val_ = BitVal::Dominant;
                    r1_driver->val_ = BitVal::Dominant;
                    srr_monitor->val_ = BitVal::Dominant;
                    r1_monitor->val_ = BitVal::Dominant;
                    break;
                }
            }

            drv_bit_frm->UpdateFrame();
            mon_bit_frm->UpdateFrame();

            mon_bit_frm->ConvRXFrame();

            /**********************************************************************************
             * Execute test
             *********************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            FreeTestObjects();
            return FinishElemTest();
        }
};