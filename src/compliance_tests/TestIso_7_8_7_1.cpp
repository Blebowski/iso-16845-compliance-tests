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
 * @date 19.6.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.7.1
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there are additional recessive
 *        to dominant edges between synchronization segment and sample point
 *        on bit position “res” bit.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *          Glitch between synchronization segment and sample point.
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the second TQ of “res” bit to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  Additionally, the Phase_Seg2(N) of “res” bit shall be forced to recessive.
 *
 * Response:
 *  The modified “res” bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_7_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force second TQ of res bit to recessive.
             *   3. Force Phase2 of res bit to recessive.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            // Res post EDL in model we mark r0 as in original CAN FD 1.0 by Bosch.
            Bit *res_bit = driver_bit_frm->GetBitOf(0, BitType::R0);
            res_bit->ForceTimeQuanta(1, BitValue::Recessive);
            res_bit->ForceTimeQuanta(0, nominal_bit_timing.ph2_ - 1,
                                     BitPhase::Ph2, BitValue::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Glitch filtering test for positive phase error on res bit");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};