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
 * @test ISO16845 7.7.8
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there are two recessive to
 *        dominant edges between two sample points where the first edge comes
 *        before the synchronization segment. The test also verifies that an
 *        IUT is able to synchronize on a minimum duration pulse obeying to
 *        the synchronization rules.
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *      Glitch pulse length = 1 TQ(N)
 *          FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 Recessive glitch at third TQ(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in arbitration field.
 *  The recessive bit before the stuff bit is shortened by one time quantum.
 *  After the first two time quanta of dominant value, it changes one time
 *  quantum to recessive value according to elementary test cases. This dominant
 *  stuff bit is followed by 6 recessive bits.
 *
 * Response:
 *  The IUT shall respond with an error frame exactly 7 bit times after the
 *  first recessive to dominant edge of the stuff bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_8 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTest(TestVariant::Common, ElementaryTest(1));

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
             *   1. Monitor frame as if received!
             *   2. Shorten bit before first stuff bit by 1 time quantum! Flip on both driven and
             *      monitored frame since DUT will re-sync. by 1!
             *   3. Flip third Time quanta of first stuff bit in arbitration field to recessive!
             *   4. ID contains all recessive. To reach sequence of 6 recessive bits, flip next
             *      stuff bit (2nd)
             *   5. Insert expected Error frame exactly after 6 bits after the end of first stuff
             *      bit (bit after 2nd stuff bit which had flipped value!). Insert Passive Error
             *      frame on driven frame so driver transmitts all recessive!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(4, BitType::BaseIdentifier)
                ->ShortenPhase(BitPhase::Ph2, 1);
            monitor_bit_frm->GetBitOf(4, BitType::BaseIdentifier)
                ->ShortenPhase(BitPhase::Ph2, 1);

            Bit *first_stuff_bit = driver_bit_frm->GetStuffBit(0);
            first_stuff_bit->GetTimeQuanta(2)->ForceValue(BitValue::Recessive);

            Bit *second_stuff_bit = driver_bit_frm->GetStuffBit(1);
            second_stuff_bit->bit_value_ = BitValue::Recessive;

            int index = driver_bit_frm->GetBitIndex(second_stuff_bit);
            driver_bit_frm->InsertActiveErrorFrame(index + 1);
            monitor_bit_frm->InsertActiveErrorFrame(index + 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing glitch filtering on negative phase error!");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};