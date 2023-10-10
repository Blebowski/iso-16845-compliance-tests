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

#include "pli_lib.h"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"
#include "../test_lib/TestLoader.h"
#include "../test_lib/ElementaryTest.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;
using namespace test_lib;

class TestIso_7_1_5 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::OneToOne);

            int num_elem_tests;
            FrameType frame_type;
            if (test_variants[0] == TestVariant::Can_2_0)
            {
                num_elem_tests = 7;
                frame_type = FrameType::Can2_0;
            }
            else if (test_variants[0] == TestVariant::CanFdTolerant)
            {
                num_elem_tests = 3;
                frame_type = FrameType::Can2_0;
            }
            else
            {
                num_elem_tests = 3;
                frame_type = FrameType::CanFd;
            }

            for (int i = 0; i < num_elem_tests; i++)
                AddElemTest(test_variants[0], ElementaryTest(i + 1, frame_type));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_,
                            IdentifierType::Extended, RtrFlag::DataFrame);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force bits per Test variant or elementary test.
             *   2. Update frames (needed since CRC might have changed).
             *   3. Turned monitored frame received.
             *************************************************************************************/

            if (test_variant == TestVariant::Can_2_0)
            {
                Bit *srr_driver = driver_bit_frm->GetBitOf(0, BitType::Srr);
                Bit *srr_monitor = monitor_bit_frm->GetBitOf(0, BitType::Srr);
                Bit *r0_driver = driver_bit_frm->GetBitOf(0, BitType::R0);
                Bit *r0_monitor = monitor_bit_frm->GetBitOf(0, BitType::R0);

                /* In CAN 2.0 Extended frame format, R1 is at position of FDF */
                Bit *r1_driver = driver_bit_frm->GetBitOf(0, BitType::R1);
                Bit *r1_monitor = monitor_bit_frm->GetBitOf(0, BitType::R1);
                
                switch (elem_test.index_)
                {
                case 1:
                    srr_driver->bit_value_ = BitValue::Recessive;
                    srr_monitor->bit_value_ = BitValue::Recessive;
                    r0_driver->bit_value_ = BitValue::Recessive;
                    r0_monitor->bit_value_ = BitValue::Recessive;
                    r1_driver->bit_value_ = BitValue::Recessive;
                    r1_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 2:
                    srr_driver->bit_value_ = BitValue::Recessive;
                    srr_monitor->bit_value_ = BitValue::Recessive;
                    r0_driver->bit_value_ = BitValue::Recessive;
                    r0_monitor->bit_value_ = BitValue::Recessive;
                    r1_driver->bit_value_ = BitValue::Dominant;
                    r1_monitor->bit_value_ = BitValue::Dominant;
                    break;
                case 3:
                    srr_driver->bit_value_ = BitValue::Recessive;
                    srr_monitor->bit_value_ = BitValue::Recessive;
                    r0_driver->bit_value_ = BitValue::Dominant;
                    r0_monitor->bit_value_ = BitValue::Dominant;
                    r1_driver->bit_value_ = BitValue::Recessive;
                    r1_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 4:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r0_driver->bit_value_ = BitValue::Recessive;
                    r0_monitor->bit_value_ = BitValue::Recessive;
                    r1_driver->bit_value_ = BitValue::Recessive;
                    r1_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 5:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r0_driver->bit_value_ = BitValue::Recessive;
                    r0_monitor->bit_value_ = BitValue::Recessive;
                    r1_driver->bit_value_ = BitValue::Dominant;
                    r1_monitor->bit_value_ = BitValue::Dominant;
                    break;
                case 6:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r0_driver->bit_value_ = BitValue::Dominant;
                    r0_monitor->bit_value_ = BitValue::Dominant;
                    r1_driver->bit_value_ = BitValue::Recessive;
                    r1_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 7:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r0_driver->bit_value_ = BitValue::Dominant;
                    r0_monitor->bit_value_ = BitValue::Dominant;
                    r1_driver->bit_value_ = BitValue::Dominant;
                    r1_monitor->bit_value_ = BitValue::Dominant;
                default:
                    break;
                }

            } else if (test_variant == TestVariant::CanFdTolerant) {
                Bit *srr_driver = driver_bit_frm->GetBitOf(0, BitType::Srr);
                Bit *srr_monitor = monitor_bit_frm->GetBitOf(0, BitType::Srr);
                Bit *r0_driver = driver_bit_frm->GetBitOf(0, BitType::R0);
                Bit *r0_monitor = monitor_bit_frm->GetBitOf(0, BitType::R0);

                switch (elem_test.index_)
                {
                case 1:
                    srr_driver->bit_value_ = BitValue::Recessive;
                    srr_monitor->bit_value_ = BitValue::Recessive;
                    r0_driver->bit_value_ = BitValue::Recessive;
                    r0_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 2:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r0_driver->bit_value_ = BitValue::Recessive;
                    r0_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 3:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r0_driver->bit_value_ = BitValue::Dominant;
                    r0_monitor->bit_value_ = BitValue::Dominant;
                    break;
                default:
                    break;
                }

            } else if (test_variant == TestVariant::CanFdEnabled) {
                Bit *srr_driver = driver_bit_frm->GetBitOf(0, BitType::Srr);
                Bit *srr_monitor = monitor_bit_frm->GetBitOf(0, BitType::Srr);
                /* R1 bit in CAN FD frames is RRS bit position */
                Bit *r1_driver = driver_bit_frm->GetBitOf(0, BitType::R1);
                Bit *r1_monitor = monitor_bit_frm->GetBitOf(0, BitType::R1);
                switch (elem_test.index_)
                {
                case 1:
                    srr_driver->bit_value_ = BitValue::Recessive;
                    r1_driver->bit_value_ = BitValue::Recessive;
                    srr_monitor->bit_value_ = BitValue::Recessive;
                    srr_driver->bit_value_ = BitValue::Recessive;
                    break;
                case 2:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    r1_driver->bit_value_ = BitValue::Recessive;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r1_monitor->bit_value_ = BitValue::Recessive;
                    break;
                case 3:
                    srr_driver->bit_value_ = BitValue::Dominant;
                    r1_driver->bit_value_ = BitValue::Dominant;
                    srr_monitor->bit_value_ = BitValue::Dominant;
                    r1_monitor->bit_value_ = BitValue::Dominant;
                    break;
                }
            }

            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            monitor_bit_frm->TurnReceivedFrame();

            /********************************************************************************** 
             * Execute test
             *********************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};