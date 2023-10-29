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
 * @date 10.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.7.6
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a negative phase error e on a recessive to dominant edge
 *        with |e| > SJW(N).
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *      #1 The values tested for e are measured in time quanta with
 *          |e| ∈ {[(SJW(N) + 1], Phase_Seg2(N)}.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT shortens the last recessive bit before an expected dominant stuff
 *  bit in arbitration field by an amount of |e| time quanta and then sends
 *  a dominant value for one time quantum followed by a recessive state
 *  according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame 1 bit time + [|e| − SJW(N)] time
 *  quanta after the last recessive to dominant edge.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_6 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            size_t num_elem_tests = nominal_bit_timing.ph2_ - nominal_bit_timing.sjw_;
            for (size_t i = 0; i < num_elem_tests; i++)
            {
                ElementaryTest test = ElementaryTest(i + 1);
                test.e_ = nominal_bit_timing.sjw_ + i + 1;
                elem_tests[0].push_back(test);
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, IdentifierType::Base);

            // Base ID full of 1s, 5th will be dominant stuff bit!
            int id = pow(2,11) - 1;
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, id);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Shorten TSEG2 of bit before first stuff bit by e in driven frame. In monitored
             *      frame, shorten only by SJW as this corresponds to how DUT has resynchronised!
             *   2. Set bit value of Dominant stuff bit to Recessive apart from 1 TQ in the
             *      beginning of the bit for driven frame!
             *   3. Insert expected error frame one bit after first stuff bit! Since bit before
             *      stuff bit was shortened by SJW, start of error frame in monitored frame should
             *      be at exact position as DUT should transmit it! Insert Passive Error frame to
             *      driver so that it sends all recessive values!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(4, BitType::BaseIdentifier)
                ->ShortenPhase(BitPhase::Ph2, elem_test.e_);
            monitor_bit_frm->GetBitOf(4, BitType::BaseIdentifier)
                ->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);

            Bit *stuff_bit = driver_bit_frm->GetStuffBit(0);
            stuff_bit->bit_value_ = BitValue::Recessive;
            stuff_bit->GetTimeQuanta(0)->ForceValue(BitValue::Dominant);

            int index = driver_bit_frm->GetBitIndex(stuff_bit);
            monitor_bit_frm->InsertActiveErrorFrame(index + 1);
            driver_bit_frm->InsertPassiveErrorFrame(index + 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing negative phase error: %d", elem_test.e_);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};