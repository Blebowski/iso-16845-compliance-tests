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
 * @date 21.12.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.8.2.2
 *
 * @brief The purpose of this test is to verify the secondary sample point of
 *        an IUT will not be applied on bit position BRS if the IUT acts as a
 *        transmitter with a delay, d, between transmitted and received signal.
 * @version CAN FD enabled
 *
 * Test variables:
 *      Available configuration methods for delay compensation = fix programmed
 *      or automatically measured.
 *          Delay, d, in range of TQ (D) = d ∈ (1, 2 data bit times)
 *          “res” bit
 *          BRS = 1
 *          FDF = 1
 *
 * Elementary test cases:
 *  There are two elementary tests to perform for 1 bit rate configuration and
 *  each way of configuration of delay compensation - fix programmed or
 *  automatically measured, shall be checked.
 *      #1 d = 1 data bit times
 *      #2 d = 2 data bit times
 *
 *  Test for late Sampling_Point(N):
 *      bit level changed after sampling point to wrong value.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation shall be enabled. SSP offset shall be
 *  configured to evaluate the delayed bit on similar position like the
 *  sampling point in data phase [Sampling_Point(D)].
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame with recessive BRS bit.
 *  The LT prolonged the SOF bit on IUT receive input by an amount of d
 *  according to elementary test cases to shift the IUT received sequence
 *  relative against the transmitted sequence of IUT.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The frame is invalid. An error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_2_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);

            /*
             * Test defines only two elementary tests, but each type of SSP shall be tested.
             * We have options: Offset, Offset + Measured. This gives us two options for each
             * elementary test, together 4 tests.
             */
            for (size_t i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1));

            // Following constraint is not due to model or IUT issues.
            // It is due to principle of the test, we can't avoid it!
            // This is because we are delaying received sequence by up to: 2 x Bit time (D).
            // If such big delay is applied, and TSEG1(N) is smaller than this number, an
            // error frame is detected still in Nominal Bit-rate.
            TEST_ASSERT(dbt.GetBitLenCycles() * 2 < ((nbt.ph1_ + nbt.prop_ + 1) * nbt.brp_),
                        " In this test TSEG1(N) > 2 * Bit time(D) due to test architecture!");

            TEST_ASSERT(dbt.GetBitLenCycles() * 3 < dut_max_secondary_sample,
                        "Bit time (N) * 3 < Limit for maximal Secondary sample point compensation!");

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift,
                                                       EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Delay received sequence by d data bit times:
             *          Elem test 1,2 : d = 1
             *          Elem test 3,4 : d = 2
             *      This is done by prolonging SOF of driven frame.
             *   2. Force BRS in shifted frame to dominant for Sync + Prop + PH1 - d. Note that d
             *      is measured in cycles, not time quantas, therefore we must count cycle by cycle!
             *   3. Insert Active Error frame to monitored and driven frames from ESI bit!
             *   4. Append retransmitted frame by IUT!
             *************************************************************************************/
            size_t d = dbt.GetBitLenCycles();
            if (elem_test.index_ == 3 || elem_test.index_ == 4)
                d *= 2;
            drv_bit_frm->GetBit(0)->GetTQ(0)->Lengthen(d);

            size_t force_cycles = nbt.brp_ *
                (nbt.prop_ + nbt.ph1_ + 1) - d;

            Bit *brs = drv_bit_frm->GetBitOf(0, BitKind::Brs);
            auto tq = brs->GetTQIter(0);
            do
            {
                if (force_cycles > tq->getLengthCycles())
                {
                    tq->ForceVal(BitVal::Dominant);
                    force_cycles -= tq->getLengthCycles();
                } else {
                    for (size_t i = 0; i < force_cycles; i++)
                        tq->ForceCycleValue(i, BitVal::Dominant);
                    force_cycles = 0;
                }
                tq++;
            } while (force_cycles > 0);

            drv_bit_frm->InsertActErrFrm(0, BitKind::Esi);
            mon_bit_frm->InsertActErrFrm(0, BitKind::Esi);

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/

            /* Reconfigure SSP: Test 1, 3 -> Measured + Offset, Test 2, 4 -> Offset only */
            dut_ifc->Disable();
            if (elem_test.index_ == 1 || elem_test.index_ == 3)
            {
                /* Offset as if normal sample point, TX/RX delay will be measured and added
                 * by IUT. Offset in clock cycles! (minimal time quanta)
                 */
                size_t ssp_offset = dbt.brp_ * (dbt.prop_ + dbt.ph1_ + 1);
                dut_ifc->ConfigureSsp(SspType::MeasAndOffset, static_cast<int>(ssp_offset));
            } else {
                /* We need to incorporate d into the delay! */
                size_t ssp_offset = dbt.brp_ * (dbt.prop_ + dbt.ph1_ + 1) + d;
                dut_ifc->ConfigureSsp(SspType::Offset, static_cast<int>(ssp_offset));
            }
            dut_ifc->Enable();
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(2000);

            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};