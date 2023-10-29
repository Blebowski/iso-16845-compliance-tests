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
 * @date 4.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.7.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a negative phase error e on a recessive to dominant edge
 *        with |e| ≤ SJW(N) on bit position ACK.
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *   Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             |e| ∈ [1, SJW(N)].
 *
 *      Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame.
 *  The LT forces an amount of e TQ from end of CRC delimiter bit to dominant.
 *  Additionally, the ACK bit shall be forced to recessive from end of bit
 *  toward Sampling_Point(N) for Phase_Seg2(N) + e according to elementary
 *  test cases. The bit shall be sampled as dominant.
 *
 * Response:
 *  The frame is valid, no error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_11 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            for (size_t i = 1; i <= nbt.sjw_; i++)
            {
                ElemTest test = ElemTest(i);
                test.e_ = i;
                AddElemTest(TestVariant::Common, std::move(test));
            }
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force last e TQ of PH2 phase of CRC Delimiter by e in driven frame.
             *   3. Shorten monitored bit of CRC delimiter by e, this is by how much IUT should
             *      resynchronize!
             *   4. Force last PH2 + e of ACK to recessive in driven frame.
             *
             * Note: This is not exactly sequence as described in ISO, there bits are not shortened
             *       but flipped, but overall effect is the same!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            auto tq_it = drv_bit_frm->GetBitOf(0, BitKind::CrcDelim)
                            ->GetLastTQIter(BitPhase::Ph2);
            for (int i = 0; i < elem_test.e_; i++)
            {
                tq_it->ForceVal(BitVal::Dominant);
                tq_it--;
            }

            mon_bit_frm->GetBitOf(0, BitKind::CrcDelim)
                ->ShortenPhase(BitPhase::Ph2, elem_test.e_);

            Bit *ack = drv_bit_frm->GetBitOf(0, BitKind::Ack);
            ack->val_ = BitVal::Dominant;

            tq_it = ack->GetLastTQIter(BitPhase::Ph2);
            for (size_t i = 0; i < elem_test.e_ + nbt.ph2_; i++)
            {
                tq_it->ForceVal(BitVal::Recessive);
                tq_it--;
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ACK negative phase error: %d", elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};