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
 * @test ISO16845 8.8.4.2
 *
 * @brief The purpose of this test is to verify that there is no synchroni-
 *        zation within 1 bit time if there are two recessive to dominant
 *        edges between two sample points where the first edge comes before
 *        the synchronization segment.
 * @version CAN FD enabled
 *
 * Test variables:
 *  CAN FD enabled
 *
 *  Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      DATA field
 *      FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform for at least 1 bit rate
 *  configuration.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces the last TQ of Phase_Seg2(D) of a recessive bit to dominant.
 *  The LT forces a following recessive bit to dominant from sync-segment up to
    Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) − 1TQ(D).

 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The frame is valid. No error flag shall occur
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_4_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            AddElemTest(TestVariant::CanFdEna, ElemTest(1));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            SetupMonitorTxTests();

            assert(dbt.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, RtrFlag::Data,
                                                        BrsFlag::DoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0xF);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Pick random recessive bit in data field which is followed by recessive bit.
             *   3. Force last TQ of picked bit to dominant.
             *   4. Force next bit from 2nd time quanta till one time quanta before sample point
             *      to dominant.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *random_bit;
            Bit *next_bit;
            do {
                random_bit = drv_bit_frm->GetRandBitOf(BitKind::Data);
                int bit_index = drv_bit_frm->GetBitIndex(random_bit);
                next_bit = drv_bit_frm->GetBit(bit_index + 1);
            } while (! (random_bit->val_ == BitVal::Recessive &&
                        next_bit->val_ == BitVal::Recessive));

            random_bit->ForceTQ(dbt.ph2_ - 1, BitPhase::Ph2, BitVal::Dominant);

            // Note: ISO here says that this bit should be forced from SYNC. But that is clearly
            //       an error, because then there would not be two recessive to dominant edges!
            //       This should be reported to ISO! It should be forces from first bit of
            for (size_t i = 1; i < dbt.prop_ + dbt.ph1_; i++)
                next_bit->ForceTQ(i, BitVal::Dominant);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};