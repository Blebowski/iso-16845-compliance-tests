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
 * @date 26.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.20
 *
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when detecting a dominant bit at the last bit of an overload delimiter
 *        it is transmitting.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 *
 *  CAN FD Enabled
 *      REC, FDF = 1
 *
 * Elementary test cases:
 *      #1 It corrupts the last bit of the overload delimiter.
 *
 * Setup:
 *  DontShift action required, the IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  Then LT applies an error according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be zero.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_20 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received, insert ACK to driven frame.
             *   2. Force last bit of EOF to DOMINANT.
             *   3. Insert expected overload frame from first bit of Intermission.
             *   4. Force 8-th bit of overload delimiter to dominant!
             *   5. Insert next expected overload frame from first bit of Intermission
             *************************************************************************************/
            monitor_bit_frm->ConvRXFrame();

            driver_bit_frm->GetBitOf(6, BitKind::Eof)->val_ = BitVal::Dominant;

            monitor_bit_frm->InsertOvrlFrm(0, BitKind::Interm);
            driver_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

            driver_bit_frm->GetBitOf(7, BitKind::OvrlDelim)->val_ = BitVal::Dominant;

            monitor_bit_frm->InsertOvrlFrm(0, BitKind::Interm);
            driver_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);

            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);
            CheckRecChange(rec_old, +0);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};