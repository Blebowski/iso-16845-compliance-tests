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
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e ≤ SJW(N).
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *      #1 The values tested for e are measured in time quanta with
 *          e ∈ [1, SJW(N)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT delays a dominant stuff bit in arbitration field by an amount of
 *  e time quanta and shortens the same bit by an amount of
 *  [Phase_Seg2(N) + 1TQ] according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame 1 bit time after the recessive to
 *  dominant edge of the delayed stuff bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            for (size_t i = 0; i < nbt.sjw_; i++){
                ElemTest test = ElemTest(i + 1);
                test.e_ = i + 1;
                elem_tests[0].push_back(test);
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, IdentKind::Base);

            /* Base ID full of 1s, 5th will be dominant stuff bit! */
            int id = pow(2,11) - 1;
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Prolong TSEG2 of bit before the stuff bit on 5th bit of identifier (delay stuff
             *      bit) by e in both driven and monitored frames.
             *   2. Force whole TSEG2 and last time quanta of TSEG1 to Recessive. This corresponds
             *      to shortening the bit by TSEG2 + 1.
             *   3. Insert Expected active error frame to be monitored on bit after stuff bit.
             *      Since also monitored bit before stuff bit was prolonged, error frame will be
             *      exactly at expected position! On driver, passive error frame so that it
             *      transmitts all recessive!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *before_stuff_bit = drv_bit_frm->GetBitOf(4, BitKind::BaseIdent);
            before_stuff_bit->LengthenPhase(BitPhase::Ph2, elem_test.e_);
            before_stuff_bit = mon_bit_frm->GetBitOf(4, BitKind::BaseIdent);
            before_stuff_bit->LengthenPhase(BitPhase::Ph2, elem_test.e_);

            Bit *stuff_bit = drv_bit_frm->GetStuffBit(0);
            for (size_t j = 0; j < nbt.ph2_; j++)
                stuff_bit->ForceTQ(j, BitPhase::Ph2, BitVal::Recessive);
            BitPhase previous_phase = stuff_bit->PrevBitPhase(BitPhase::Ph2);
            stuff_bit->GetLastTQIter(previous_phase)
                ->ForceVal(BitVal::Recessive);

            int index = drv_bit_frm->GetBitIndex(stuff_bit);
            mon_bit_frm->InsertActErrFrm(index + 1);
            drv_bit_frm->InsertPasErrFrm(index + 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckNoRxFrame();

            return FinishElemTest();
        }
};
