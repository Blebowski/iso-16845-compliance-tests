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
 * @date 20.6.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.8.3
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there is an additional recessive
 *        to dominant edge between two sample points where the first edge comes
 *        before the synchronization segment on bit position ACK.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Bit start with negative offset and glitch between synchronization
 *      segment and sample point.
 *          ACK
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *      #1 The LT reduce the length of CRC delimiter bit by one TQ(D) and the
 *         LT force the second TQ of ACK bit to Recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
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

class TestIso_7_8_8_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            AddElemTest(TestVariant::CanFdEna, ElemTest(1));

            // Note: In this TC TX to RX feedback cant be enabled, since DUT
            //       would corrupt test pattern by IUT in ACK field!
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
             *   2. Shorten CRC delimiter by 1 TQ in both driven and monitored frames.
             *   3. Force 2nd TQ of driven ACK bit to Recessive.
             *   4. Force whole Phase 2 of ACK bit to Recessive.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *crc_delim_driver = drv_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            Bit *crc_delim_monitor = mon_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            Bit *ack_bit = drv_bit_frm->GetBitOf(0, BitKind::Ack);

            // ACK must be sent dominant since TX/RX feedback is not turned on!
            ack_bit->val_ = BitVal::Dominant;

            crc_delim_driver->ShortenPhase(BitPhase::Ph2, 1);
            crc_delim_monitor->ShortenPhase(BitPhase::Ph2, 1);

            ack_bit->ForceTQ(1, BitVal::Recessive);
            ack_bit->ForceTQ(0, nbt.ph2_ - 1, BitPhase::Ph2, BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ACK bit glitch filtering on negative phase error");
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);

            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};