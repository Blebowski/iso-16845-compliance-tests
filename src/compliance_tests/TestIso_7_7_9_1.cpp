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
 * @date 3.5.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.7.9.1
 *
 * @brief The purpose of this test is to verify that an IUT will not detect an
 *        SOF when detected dominant level ≤ [Prop_Seg(N) + Phase_Seg1(N) −
 *        1 TQ(N)].
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *  Glitch Pulse length = Prop_Seg(N) + Phase_Seg1(N) − 1 TQ(N)
 *      FDF = 0
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *      #1 Dominant pulse on IDLE bus [Prop_Seg(N) + Phase_Seg1(N) − 1 TQ(N)].
 *      Refer to 6.2.3.
 *
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a dominant glitch according to elementary test cases for
 *  this test case. Then the LT waits for 8 bit times.
 *
 * Response:
 *  The IUT shall remain in the idle state.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_7_9_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTest(TestVariant::Common, ElemTest(1));
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // CAN 2.0 frame, randomize others
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Remove all bits but first from monitored frame.
             *   2. Remove all bits but first from driven frame.
             *   3. Shorten SOF to PROP + PH1 - 1 Time quanta in driven frame.
             *   4. Insert 9 recessive bits to monitor.
             *************************************************************************************/
            drv_bit_frm->RemoveBitsFrom(1);
            mon_bit_frm->RemoveBitsFrom(1);
            mon_bit_frm->GetBit(0)->val_ = BitVal::Recessive;

            drv_bit_frm->GetBit(0)->ShortenPhase(BitPhase::Ph2, nbt.ph2_);
            drv_bit_frm->GetBit(0)->ShortenPhase(BitPhase::Sync, 1);
            BitPhase phase = drv_bit_frm->GetBit(0)->PrevBitPhase(BitPhase::Ph2);
            drv_bit_frm->GetBit(0)->ShortenPhase(phase, 1);

            for (int i = 0; i < 9; i++)
            {
                mon_bit_frm->InsertBit(BitKind::Sof, BitVal::Recessive, 1);
                drv_bit_frm->InsertBit(BitKind::Sof, BitVal::Recessive, 1);
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Glitch filtering in idle state - single glitch");
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            return FinishElemTest();
        }
};