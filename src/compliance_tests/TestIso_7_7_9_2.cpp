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
 * @date 3.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.7.9.2
 *
 * @brief The purpose of this test is to verify that an IUT will not use any
 *        edge for resynchronization after detection of a recessive to dominant
 *        edge in idle state (after hard synchronization).
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *  Dominant pulses on IDLE bus. Pulse group:
 *      a) First glitch = (Prop_Seg(N) + Phase_Seg1(N) − 2)/2
 *      b) Recessive time = 2 TQ(N)
 *      c) Second glitch = {[Prop_Seg(N) + Phase_Seg1(N) - 2]/2} − 1 minimum
 *         time quantum.
 *      d) Recessive time = 1 TQ(N) + 2 minimum time quantum
 *      e) Third glitch = Prop_Seg(N) + Phase_Seg1(N) – 2
 *  FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 Three dominant glitches separated by recessive TQ(N) times.
 *             The first glitch activates the edge detection of IUT. The next
 *             two glitches cover the TQ(N) position of the configured
 *             Sampling_Point(N) regarding to the first glitch.
 *      Refer to 6.2.3
 *
 * Setup:
 *  DontShift action required, the IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a dominant glitch group according to elementary test cases
 *  for this test case.
 *  Then, the LT waits for 8 bit times to check that no error frame will start
 *  after that.
 *
 * Response:
 *  The IUT shall remain in the idle state.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_9_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTest(TestVariant::Common, ElementaryTest(1));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // CAN 2.0 frame, randomize others
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Remove all bits but first 6 from driven frame.
             *   2. Set value of first 5 bits to be corresponding to glitches. Modify length of
             *      bits to each correspond to one glitch space between.
             *   3. Remove all bits but first from monitored frame.
             *   4. Insert 9 recessive bits to monitor.
             *************************************************************************************/
            driver_bit_frm->RemoveBitsFrom(6);

            // Set values
            driver_bit_frm->GetBit(0)->bit_value_ = BitValue::Dominant;
            driver_bit_frm->GetBit(1)->bit_value_ = BitValue::Recessive;
            driver_bit_frm->GetBit(2)->bit_value_ = BitValue::Dominant;
            driver_bit_frm->GetBit(3)->bit_value_ = BitValue::Recessive;
            driver_bit_frm->GetBit(4)->bit_value_ = BitValue::Dominant;
            driver_bit_frm->GetBit(5)->bit_value_ = BitValue::Recessive;

            // Set glitch lengths

            // First reduce other phases, we create glitches it from SYNC!
            for (int i = 0; i < 5; i++)
            {
                printf("Setting bit %d\n", i);
                driver_bit_frm->GetBit(i)->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
                driver_bit_frm->GetBit(i)->ShortenPhase(BitPhase::Ph1, nominal_bit_timing.ph2_);
                driver_bit_frm->GetBit(i)->ShortenPhase(BitPhase::Prop, nominal_bit_timing.ph2_);
            }

            // Now set to length as in description. SYNC already has length of one!
            driver_bit_frm->GetBit(0)->LengthenPhase(BitPhase::Sync,
                (nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ - 2) / 2 - 1);

            driver_bit_frm->GetBit(1)->LengthenPhase(BitPhase::Sync, 1);

            driver_bit_frm->GetBit(2)->LengthenPhase(BitPhase::Sync,
                (nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ - 2) / 2 - 1);
            driver_bit_frm->GetBit(2)->GetTimeQuanta(0)->Shorten(1);

            driver_bit_frm->GetBit(3)->GetTimeQuanta(0)->Lengthen(2);

            driver_bit_frm->GetBit(4)->LengthenPhase(BitPhase::Sync,
                nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ - 3);

            // Passive error frame consists of all recessive so this monitors unit
            // will not start transmitting active error frame!
            monitor_bit_frm->GetBit(0)->bit_value_ = BitValue::Recessive;
            monitor_bit_frm->InsertPassiveErrorFrame(monitor_bit_frm->GetBit(1));

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Glitch filtering in idle state - single glitch");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};