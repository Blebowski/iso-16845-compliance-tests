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
 * @date 2.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.7.1
 *
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *      FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT shortens a dominant stuff bit in arbitration field by an amount of
 *  Phase_Seg2(N) and then later shortens another dominant stuff bit by an
 *  amount of [Phase_Seg2(N) + 1] according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame on the bit position following the
 *  second shortened stuff bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTest(TestVariant::Common, ElemTest(1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, IdentKind::Base);

            /* Base ID full of 1s */
            int id = pow(2,11) - 1;
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Shorten 6-th bit (1st stuff bit) of driven frame by PhaseSeg2.
             *   3. Shorten 12-th bit (2nd stuff bit) of driven frame still in Base ID by
             *      PhaseSeg2 + 1.
             *   4. Correct lenght of one of the monitored bits since second stuff bit causes
             *      negative re-synchronization.
             *   5. Insert Error frame from 12-th bit of Identifier (5 + Stuff + 5 + Stuff).
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *first_stuff_bit = drv_bit_frm->GetStuffBit(0);
            first_stuff_bit->ShortenPhase(BitPhase::Ph2, nbt.ph2_);

            Bit *second_stuff_bit = drv_bit_frm->GetStuffBit(1);
            second_stuff_bit->ShortenPhase(BitPhase::Ph2, nbt.ph2_);
            BitPhase previous_phase = second_stuff_bit->PrevBitPhase(BitPhase::Ph2);
            second_stuff_bit->ShortenPhase(previous_phase, 1);

            /* Compensate the monitored frame as if negative resynchronisation happend.
             * This is due to first bit being shortened by PH2.
             */
            size_t resync_amount = nbt.ph2_;
            if (nbt.sjw_ < resync_amount)
                resync_amount = nbt.sjw_;
            mon_bit_frm->GetBitOf(11, BitKind::BaseIdent)
                ->ShortenPhase(BitPhase::Ph2, resync_amount);

            /*
             * Expected error frame on monitor (from start of bit after stuff bit)
             * Driver will have passive error frame -> transmitt all recessive
             */
            mon_bit_frm->InsertActErrFrm(12, BitKind::BaseIdent);
            drv_bit_frm->InsertPasErrFrm(12, BitKind::BaseIdent);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckNoRxFrame();

            FreeTestObjects();
            return FinishElemTest();
        }
};