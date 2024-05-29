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
 * @test ISO16845 7.8.1.3
 *
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *          CRC Delimiter
 *          FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform for at least 1 bit rate
 *      configuration:
 *          #1 test for early sampling point: bit level change to recessive
 *             before sampling point;
 *          #2 test for late sampling point: bit level change to recessive
 *             after sampling point.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  Test CRC delimiter #1:
 *      The LT forces a recessive CRC delimiter bit to dominant from beginning
 *      up to one TQ(D) before the Sampling point.
 *
 *  Test CRC delimiter #2:
 *      The LT forces a recessive CRC delimiter bit to dominant from beginning
 *      up to the sampling point.
 *
 * Response:
 *  Test CRC delimiter #1:
 *      The modified CRC delimiter bit shall be sampled as recessive.
 *      The frame is valid. DontShift error flag shall occur.
 *
 *  Test CRC delimiter #2:
 *      The modified CRC delimiter bit shall be sampled as dominant.
 *      The frame is invalid. An error frame shall follow.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_1_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = 0; i < 2; i++) {
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::Can20));
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
             *   2. Modify CRC Delimiter, flip TSEG1 - 1 (elementary test 1) or TSEG1 (elementary
             *      test 2) to dominant!
             *   3. For elementary test 2, insert active error frame right after CRC
             *      delimiter! Insert passive error frame to driver to send
             *      all recessive (TX to RX feedback is turned ON)!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *crc_delim = drv_bit_frm->GetBitOf(0, BitKind::CrcDelim);
            size_t bit_index = drv_bit_frm->GetBitIndex(crc_delim);
            size_t dominant_pulse_lenght;

            if (elem_test.index_ == 1)
                dominant_pulse_lenght = dbt.prop_ + dbt.ph1_;
            else
                dominant_pulse_lenght = dbt.prop_ + dbt.ph1_ + 1;

            for (size_t j = 0; j < dominant_pulse_lenght; j++)
                crc_delim->ForceTQ(j, BitVal::Dominant);

            if (elem_test.index_ == 2)
            {
                drv_bit_frm->InsertPasErrFrm(bit_index + 1);
                mon_bit_frm->InsertActErrFrm(bit_index + 1);
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            if (elem_test.index_ == 1)
                TestMessage("Testing CRC delimiter bit sampled Recessive");
            else
                TestMessage("Testing CRC delimiter bit sampled Dominant");

            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            // Read received frame from DUT and compare with sent frame
            // (first elementary test only, second one ends with error frame)
            if (elem_test.index_ == 1)
                CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};