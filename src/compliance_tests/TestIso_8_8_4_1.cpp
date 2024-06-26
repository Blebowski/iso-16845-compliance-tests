/*****************************************************************************
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
 * @date 14.1.2021
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.8.4.1
 *
 * @brief The purpose of this test is to verify that there is no synchronization
 *        within 1 bit time if there are two recessive to dominant edges between
 *        two sample points where the first edge comes before the
 *        synchronization segment.
 * @version CAN FD enabled
 *
 * Test variables:
 *  CAN FD enabled
 *
 *  Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      ESI = 1
 *      FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform for at least 1 bit rate configuration.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  The LT force the IUT to passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces the last TQ of Phase_Seg2(D) of BRS bit to dominant.
 *  The LT forces the ESI bit to dominant from the 2nd TQ(D) for
 *  [Prop_Seg(D) + Phase_Seg1(D) − TQ(D)].
 *
 * Response:
 *  The modified ESI bit shall be sampled as recessive.
 *  The frame is valid.
 *  No error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_4_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);

            AddElemTest(TestVariant::CanFdEna, ElemTest(1));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            SetupMonitorTxTests();

            TEST_ASSERT(dbt.brp_ > 2,
                        "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift,
                                                        EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Force last TQ of BRS to dominant.
             *   3. Force ESI bit to dominant from 2nd TQ to one TQ before sample point.
             *   4. Append suspend transmission.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *brs = drv_bit_frm->GetBitOf(0, BitKind::Brs);
            brs->ForceTQ(dbt.ph2_ - 1, BitPhase::Ph2, BitVal::Dominant);

            Bit *esi = drv_bit_frm->GetBitOf(0, BitKind::Esi);
            for (size_t i = 1; i < dbt.prop_ + dbt.ph1_; i++)
                esi->ForceTQ(i, BitVal::Dominant);

            drv_bit_frm->AppendSuspTrans();
            mon_bit_frm->AppendSuspTrans();

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(150); /* To make sure IUT is error passive */
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};