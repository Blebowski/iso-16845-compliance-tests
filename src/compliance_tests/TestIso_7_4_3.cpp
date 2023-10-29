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
 * @date 2.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.4.3
 *
 * @brief This test verifies that the IUT generates an overload frame when
 *        detecting a dominant bit on the eighth bit of an error and overload
 *        delimiter it is transmitting.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Error Delimiter, Overload delimiter, FDF = 0
 *
 *  CAN FD Enabled
 *      Error Delimiter, Overload delimiter, FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 apply error at the eighth bit of the error delimiter;
 *          #2 apply error at the eighth bit of the overload delimiter.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field or an
 *  overload frame after a data frame.
 *  The LT forces 1 bit to dominant state according to elementary test cases.
 *
 * Response:
 *  The IUT generates an overload frame starting at the bit position following
 *  the dominant bit forced by LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_4_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (int i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                            IdentKind::Base, RtrFlag::Data, BrsFlag::NoShift,
                            EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &error_data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitored frame as received.
             *   2. Based on elementary test:
             *      2.1 Flip 7-th bit of data byte to dominant. This should be a recessive
             *          stuff bit. Insert active error frame from next bit on to monitored
             *          frame. Insert passive frame to driven frame (TX/RX feedback enabled).
             *      2.2 Flip first bit of intermission to dominant (overload flag).
             *          Insert expected overload frame from next bit on.
             *   3. Flip last bit of overload or error delimiter (based on previous step) to
             *      dominant.
             *   4. Insert expected overload frame from next bit on.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            if (elem_test.index_ == 1)
            {
                drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

                mon_bit_frm->InsertActErrFrm(7, BitKind::Data);
                drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            }
            else
            {
                drv_bit_frm->GetBitOf(0, BitKind::Interm)->FlipVal();

                mon_bit_frm->InsertOvrlFrm(1, BitKind::Interm);
                drv_bit_frm->InsertPasErrFrm(1, BitKind::Interm);
            }

            /* Note that driver contains only passive error flags. Overload is in monitor */
            Bit *last_delim_bit;
            last_delim_bit = drv_bit_frm->GetBitOf(7, BitKind::ErrDelim);
            last_delim_bit->FlipVal();

            int bit_index = drv_bit_frm->GetBitIndex(last_delim_bit);
            drv_bit_frm->InsertPasErrFrm(bit_index + 1);
            mon_bit_frm->InsertOvrlFrm(bit_index + 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**********************************************************************************
             * Execute test
             *********************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            if (elem_test.index_ == 1)
                CheckNoRxFrame();
            else
                CheckRxFrame(*gold_frm);

            FreeTestObjects();
            return FinishElemTest();
        }
};