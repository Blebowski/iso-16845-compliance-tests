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
 * @test ISO16845 7.8.6.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| > SJW on bit position ACK.
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
 *              |e| ∈ {[SJW(N) + 1], Phase_Seg2(N)}
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame.
 *  The LT forces an amount of |e| TQ from end of Phase_Seg2(N) of CRC
 *  delimiter bit to dominant according to elementary test cases. By this,
 *  the CRC delimiter bit of the IUT is shortened by an amount of SJW(N).
 *
 *  Additionally, the Phase_Seg2(N) of ACK bit shall be forced to recessive.
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

class TestIso_7_8_6_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = nbt.sjw_ + 1; i <= nbt.ph2_; i++)
            {
                ElemTest test = ElemTest(i - nbt.sjw_);
                test.e_ = static_cast<int>(i);
                AddElemTest(TestVariant::CanFdEna, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force last e time quanta of CRC delimiter to Dominant in driven frame.
             *   3. Shorten CRC delimiter of monitored frame by nominal SJW (this corresponds to
             *      DUTs expected resynchronisation).
             *   4. Force PH2 of ACK bit to Recessive.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *crc_delimiter_driver = drv_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            Bit *crc_delimiter_monitor = mon_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            Bit *ack_driver = drv_bit_frm->GetBitOf(0, BitKind::Ack);

            for (size_t j = 0; j < static_cast<size_t>(elem_test.e_); j++) {
                TEST_ASSERT(nbt.ph2_ >= j + 1, "'ForceTQ' will undreflow");
                crc_delimiter_driver->ForceTQ(nbt.ph2_ - 1 - j, BitPhase::Ph2, BitVal::Dominant);
            }

            crc_delimiter_monitor->ShortenPhase(BitPhase::Ph2, nbt.sjw_);

            for (size_t j = 0; j < nbt.ph2_; j++)
                ack_driver->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ACK negative resynchronisation with phase error: %d",
                         elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};