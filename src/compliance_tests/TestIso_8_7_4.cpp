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
 * @date 15.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.7.4
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT, acting
 *        as a transmitter, detecting a negative phase error e on a recessive
 *        to dominant edge with |e| ≤ SJW(N).
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 *
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Phase error e
 *      FDF = 0
 *
 * Elementary test cases:
 *  There are min{SJW(N), [Phase_Seg2(N) – IPT]} elementary tests to perform
 *  for at least 1 bit rate configuration.
 *
 *      #1 The values tested for e are measured in time quanta
 *         |e| ∈ {1, min[SJW(N)], [Phase_Seg2(N) – IPT]}.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  The LT shortens a recessive bit preceding a dominant bit by an amount of
 *  |e| inside the arbitration field according to elementary test cases.
 *
 * Response:
 *  The next edge sent by the IUT occurs an integer number of bit times after
 *  the edge applied by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);

            size_t num_elem_tests = nbt.sjw_;
            if (nbt.ph2_ < nbt.sjw_)
                num_elem_tests = nbt.ph2_;

            for (size_t i = 0; i < num_elem_tests; i++)
            {
                ElemTest test = ElemTest(i + 1);
                test.e_ = static_cast<int>(i + 1);
                AddElemTest(TestVariant::Common, std::move(test));
            }

            SetupMonitorTxTests();
            CanAgentSetWaitForMonitor(true);

            assert((nbt.brp_ > 2 &&
                    "BRP Nominal must be bigger than 2 in this test due to test architecture!"));
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Choose random recessive bit in arbitration field which is followed by dominant
             *      bit.
             *   2. Shorten PH2 of this bit by e in driven frame.
             *   3. In Monitored frame, shorten PH2 of the same bit by e - 1. Shorten SYNC seg of
             *      next bit by 1.
             *      Rationale:
             *          If edge arrives in last TQ of TSEG2, actual duration of PH2 will
             *          NOT be changed! Only sub-sequent TSEG1 will last one clock cycle less!!
             *          This corresponds to "effect of resync. is the same as of hard sync if
             *          magnitude of phase error is smaller than SJW"!!!
             *   4. Insert ACK to driven frame.
             *
             * Note: TX/RX feedback must be disabled, since we modify driven frame.
             *************************************************************************************/
            Bit *bit_to_shorten;
            Bit *next_bit;
            size_t bit_index;
            do
            {
                bit_to_shorten = drv_bit_frm->GetRandBitOf(BitKind::BaseIdent);
                bit_index = drv_bit_frm->GetBitIndex(bit_to_shorten);
                next_bit = drv_bit_frm->GetBit(bit_index + 1);
            } while (!(bit_to_shorten->val_ == BitVal::Recessive &&
                        next_bit->val_ == BitVal::Dominant));

            bit_to_shorten->ShortenPhase(BitPhase::Ph2, static_cast<size_t>(elem_test.e_));

            mon_bit_frm->GetBit(bit_index)->ShortenPhase(BitPhase::Ph2, static_cast<size_t>(elem_test.e_ - 1));
            mon_bit_frm->GetBit(bit_index + 1)->ShortenPhase(BitPhase::Sync, 1);

            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

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

            return FinishElemTest();
        }

};