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
 * @test ISO16845 7.8.9.1
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        BRS.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          BRS = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the first two TQ(N) and the complete Phase_Seg2(N)
 *             of BRS bit to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant BRS bit.
 *  The LT inverts parts of BRS bit according to elementary test cases.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur. The bit rate will not
 *  switch for data phase.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_8_9_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            AddElemTest(TestVariant::CanFdEna, ElemTest(1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Here we have to set Bit rate dont shift because we intend to get BRS dominant,
            // so bit rate should not be shifted!
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::NoShift);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Flip BRS value to dominant.
             *   3. Force first two TQ of BRS and Phase 2 of BRS to Recessive. This should cause
             *      resynchronisation edge with phase error 2, but DUT shall ignore it and not
             *      resynchronize because previous bit (r0) was Dominant!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            Bit *brs_bit = drv_bit_frm->GetBitOf(0, BitKind::Brs);

            brs_bit->val_ = BitVal::Dominant;

            brs_bit->ForceTQ(0, BitVal::Recessive);
            brs_bit->ForceTQ(1, BitVal::Recessive);

            // Force all TQ of PH2 as if no shift occured (this is what frame was generated with)
            brs_bit->ForceTQ(0, nbt.ph2_ - 1, BitPhase::Ph2, BitVal::Recessive);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("No synchronisation after dominant bit sampled on BRS bit!");
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);

            CheckLTResult();
            CheckRxFrame(*gold_frm);

            return FinishElemTest();
        }
};