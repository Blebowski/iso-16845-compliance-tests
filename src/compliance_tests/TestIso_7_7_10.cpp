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
 * @date 11.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.7.10
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        resynchronization if the value detected at the previous sample point
 *        is the same as the bus value immediately after the edge.
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *      Glitch between 2 dominant sampled bits
 *          FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 One TQ recessive glitch in Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in arbitration field.
 *  At the position [NTQ(N) - Phase_Seg2(N) + 1] time quanta after the falling
 *  edge at the beginning of the stuff bit, the LT changes the value to recessive
 *  for one time quantum according to elementary test cases.
 *  The stuff bit is followed by 5 additional dominant bits.
 *
 * Response:
 *  The IUT shall respond with an error frame exactly 6 bit times after the
 *  recessive to dominant edge at the beginning of the stuff bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_10 : public test::TestBase
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

            // Base ID - first 5 bits recessive, next 6 dominant
            // this gives ID with dominant bits after first stuff bit!
            int id = 0b11111000000;
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received!
             *   2. Flip NTQ - Ph2 + 1 time quanta of first stuff bit to recessive.
             *   3. Flip second stuff bit to dominant!
             *   4. Insert Active Error flag one bit after 2nd stuff bit! Insert Passive Error
             *      flag to driver so that it transmitts all recessive!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *first_stuff_bit = drv_bit_frm->GetStuffBit(0);
            assert (first_stuff_bit->GetLenTQ() >= nbt.ph2_ && "'tq_position' will underflow!");
            size_t tq_position = first_stuff_bit->GetLenTQ() - nbt.ph2_;
            first_stuff_bit->GetTQ(tq_position)->ForceVal(BitVal::Recessive);

            Bit *second_stuff_bit = drv_bit_frm->GetStuffBit(1);
            second_stuff_bit->val_ = BitVal::Dominant;
            size_t index = drv_bit_frm->GetBitIndex(second_stuff_bit);

            mon_bit_frm->InsertActErrFrm(index + 1);
            drv_bit_frm->InsertPasErrFrm(index + 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing glitch filtering on negative phase error!");
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            return FinishElemTest();
        }
};