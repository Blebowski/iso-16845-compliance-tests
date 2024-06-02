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
 * @date 23.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.3.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e ≤ SJW(D) on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          CRC: LSB = 1
 *          CRC delimiter
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             e ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a test frame with a recessive bit value at last bit of CRC.
 *  The LT forces the CRC delimiter to dominant bit value.
 *  Then, the recessive to dominant edge between LSB of CRC and CRC delimiter
 *  shall be delayed by additional e TQ(D)’s of recessive value at the
 *  beginning of CRC delimiter bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed CRC delimiter bit to
 *  recessive. This recessive part of Phase_seg2 start at e − 1 TQ(D) after
 *  sampling point.
 *
 * Response:
 *  The modified CRC delimiter bit shall be sampled as recessive.
 *  The frame is valid, no error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_3_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = 1; i <= dbt.sjw_; i++)
            {
                ElemTest test = ElemTest(i);
                test.e_ = static_cast<int>(i);
                AddElemTest(TestVariant::CanFdEna, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x55;
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, IdentKind::Base,
                                                       RtrFlag::Data, BrsFlag::DoShift,
                                                       EsiFlag::ErrAct);
            // Frame was empirically debugged to have last bit of CRC in 1!
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, 50, &data_byte);
            gold_frm->Print();

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force CRC delimiter of driven frame to dominant.
             *   3. Force first e bits of CRC delimiter to Recessive (This delays dominant to
             *      recessive edge by e T!) in driven frame.
             *   4. Lenghten CRC delimiter in monitored frame by e. This corresponds to how
             *      IUT should have re-synchronized!
             *   4. Shorten PH2 of CRC Delimiter to 0 since this-one is in multiples of nominal
             *      Time quanta. Lengthen PH1 (still in data time quanta), by e - 1. This has equal
             *      effect as forcing the bit to Recessive e - 1 after sample point. Next bit is
             *      ACK which is transmitted recessive by driver.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *crc_delimiter = drv_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            crc_delimiter->val_ = BitVal::Dominant;

            TEST_ASSERT(elem_test.e_ > 0, "'j' will underflow");
            for (size_t j = 0; j < static_cast<size_t>(elem_test.e_); j++)
                crc_delimiter->ForceTQ(j, BitVal::Recessive);

            mon_bit_frm->GetBitOf(0, BitKind::CrcDelim)
                ->LengthenPhase(BitPhase::Sync, static_cast<size_t>(elem_test.e_));

            crc_delimiter->ShortenPhase(BitPhase::Ph2, nbt.ph2_);
            BitPhase phase = crc_delimiter->PrevBitPhase(BitPhase::Ph2);
            crc_delimiter->LengthenPhase(phase, static_cast<size_t>(elem_test.e_) - 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing CRC Delimiter positive resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};