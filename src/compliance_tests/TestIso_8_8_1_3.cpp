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
 * @date 21.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.8.1.3
 *
 * @brief The purpose of this test is to verify the sample point of an IUT
 *        acting as a transmitter on a bit position at DATA field.
 * @version CAN FD enabled
 *
 * Test variables:
 *  CAN FD enabled
 *      Sampling_Point(D) configuration as available by IUT.
 *      DATA field
 *      BRS = 1
 *      FDF = 1
 *
 * Elementary test cases:
 *  There are two elementary tests to perform for each programmable sampling
 *  point inside a chosen number of TQ for at least 1 bit rate configuration:
 *      #1 test for late sampling point: bit level changed after sampling
 *         point to wrong value;
 *      #2 test for early sampling point: bit level changed before sampling
 *         point to correct value.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation is disabled.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *
 *  Test DATA #1:
 *      The LT forces Phase_Seg2(D) of a dominant bit to recessive.
 *
 *  Test DATA #2:
 *      The LT force a recessive bit to dominant for
 *      Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) – 1 TQ(D).
 *
 * Response:
 *  Test DATA #1:
 *      The modified data bit shall be sampled as dominant.
 *      The frame is valid. No error flag shall occur.
 *  Test DATA #2:
 *      The modified data bit shall be sampled as recessive.
 *      The frame is valid. No error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_1_3 : public test::TestBase
{
    public:
        BitTiming test_data_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);

            // Change data sample point. Iterate with sample point:
            //  from: PROP = 1, PH2 = TQ(D) - 2
            //  to: PROP = TQ(D) - 2, PH2 = 1
            for (size_t i = 0; i < (2 * dbt.GetBitLenTQ()) - 4; i += 2)
            {
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 2, FrameKind::Can20));
            }

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);

            SetupMonitorTxTests();

            assert(dbt.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Calculate new bit-rate from configured one. Have same bit-rate, but
            // different sample point. Shift sample point from 2 TQ up to 1 TQ before the
            // end.
            test_data_bit_timing.brp_ = dbt.brp_;
            test_data_bit_timing.sjw_ = dbt.sjw_;
            test_data_bit_timing.ph1_ = 0;
            test_data_bit_timing.prop_ = (elem_test.index_ + 1) / 2;
            test_data_bit_timing.ph2_ = dbt.GetBitLenTQ() -
                                        ((elem_test.index_ + 1) / 2) - 1;

            /* Re-configure bit-timing for this test so that frames are generated with it! */
            this->dbt = test_data_bit_timing;

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(nbt, test_data_bit_timing);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(100000);

            TestMessage("Data bit timing for this elementary test:");
            test_data_bit_timing.Print();


            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift,
                                                       EsiFlag::ErrAct);
            /* To make sure there is at least 1 data byte! */
            gold_frm = std::make_unique<Frame>(*frm_flags, rand() % 0xF + 1);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Choose random bit of data field of driven frame like so:
             *       - Elementary test 1 : Dominant bit
             *       - Elementary test 2 : Recessive bit
             *   3. Force parts of the generated bit like so:
             *       - Elementary test 1 : Phase 2 to Recessive.
             *       - Elementary test 2 : SYNQ+ PROP + Phase 1 - 1 TQ to Dominant.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *data_bit;
            BitVal bit_value;
            if (elem_test.index_ == 1)
                bit_value = BitVal::Dominant;
            else
                bit_value = BitVal::Recessive;

            do{
                data_bit = drv_bit_frm->GetRandBitOf(BitKind::Data);
            } while (data_bit->val_ != bit_value);

            if (elem_test.index_ == 1)
            {
                for (size_t i = 0; i < dbt.ph2_; i++)
                    data_bit->ForceTQ(i, BitPhase::Ph2, BitVal::Recessive);
            } else {
                size_t num_time_quantas = dbt.prop_ + dbt.ph1_;
                for (size_t i = 0; i < num_time_quantas; i++)
                    data_bit->ForceTQ(i, BitVal::Dominant);
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};