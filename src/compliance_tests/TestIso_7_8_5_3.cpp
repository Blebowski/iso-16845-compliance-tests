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
 * @date 16.6.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW on bit position ACK.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *          Phase error e
 *          ACK
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              |e| ∈ [1, SJW(N)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame.
 *  The LT shortened the CRC delimiter by an amount of |e| TQ according to
 *  elementary test cases.
 *  Additionally, the Phase_Seg2(N) of this dominant ACK bit shall be forced
 *  to recessive.
 *
 * Response:
 *  The modified ACK bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_5_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = 1; i <= nominal_bit_timing.sjw_; i++)
            {
                ElementaryTest test = ElementaryTest(i);
                test.e_ = i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }

            // Note: We can't enable TX to RX feedback here since DUT would
            //       screw us modified bits by transmitting dominant ACK!
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force driven ACK bit to Dominant.
             *   3. Shorten CRC delimiter of driven and monitored bits by e.
             *   4. If e = SJW, then force the first TQ of monitored bit value of ACK to
             *      Recessive. This is to compensate the fact that when e = SJW, IUT will finish
             *      immediately and shorten following TSEG1 (as if PH2 of CRC Delimiter is SYNC
             *      of ACK bit). Resynchronization is correct, but IUT will start transmitt ACK
             *      one TQ later (it will not shorten its PH2 to 0 cycles if it receives
             *      synchronization edge in first TQ of PH2 due to plain causality).
             *   5. Force Phase 2 of ACK to Recessive on driven bit!
             *************************************************************************************/
            monitor_bit_frm->ConvRXFrame();
            driver_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *crc_delimiter_driver = driver_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            Bit *crc_delimiter_monitor = monitor_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            Bit *ack_driver = driver_bit_frm->GetBitOf(0, BitKind::Ack);

            crc_delimiter_driver->ShortenPhase(BitPhase::Ph2, elem_test.e_);
            crc_delimiter_monitor->ShortenPhase(BitPhase::Ph2, elem_test.e_);

            if ((size_t)elem_test.e_ == nominal_bit_timing.sjw_)
            {
                Bit *ack_monitor = monitor_bit_frm->GetBitOf(0, BitKind::Ack);
                ack_monitor->ForceTQ(0, BitVal::Recessive);
                //ack_monitor->bit_value_ = BitValue::Recessive;
            }

            for (size_t j = 0; j < nominal_bit_timing.ph2_; j++)
                ack_driver->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ACK negative resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};