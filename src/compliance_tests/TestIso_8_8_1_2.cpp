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
 * @test ISO16845 8.8.1.2
 *
 * @brief The purpose of this test is to verify the sample point of an IUT
 *        acting as a transmitter on bit position BRS.
 * @version CAN FD enabled
 *
 * Test variables:
 *  CAN FD enabled
 *      Sampling_Point(N) configuration as available by IUT.
 *      BRS = 1
 *      FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform for each programmable sampling
 *  point inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 BRS bit level changed from dominant to recessive before sampling
 *         point.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation is disabled.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces Phase_Seg2(N) of “res” bit to recessive according to
 *  elementary test cases.
 *
 * Response:
 *  The modified “res” bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_1_2 : public test::TestBase
{
    public:
        BitTiming test_nom_bit_timing;
        BitTiming test_data_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);

            // Elementary test for each possible positon of sample point between: (2, NTQ-1)
            // Note that this test verifies BRS bit, so we need to alternate also data bit timing!
            // This will then affect overall bit-rate!
            for (size_t i = 0; i < nbt.GetBitLenTQ() - 2; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::Can20));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);

            SetupMonitorTxTests();

            assert(nbt.brp_ > 2 &&
                   "TQ(N) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Calculate new bit-rate from configured one. Modify PROP and PH2 of Nominal bit-rate
            // Modify only PH2 of data bit-rate! This will keep the same nominal bit-rate but
            // change Data bit-rate
            test_nom_bit_timing.brp_ = nbt.brp_;
            test_nom_bit_timing.sjw_ = nbt.sjw_;
            test_nom_bit_timing.ph1_ = 0;
            test_nom_bit_timing.prop_ = elem_test.index_;
            test_nom_bit_timing.ph2_ = nbt.GetBitLenTQ() - elem_test.index_ - 1;

            test_data_bit_timing.brp_ = dbt.brp_;
            test_data_bit_timing.sjw_ = dbt.sjw_;
            test_data_bit_timing.ph1_ = 0;

            // If we have BRP_FD = 1, then with index 1, TSEG1 is only 2 which is not enough, and it
            // is below minimal possible bit-rate for Data bit time! Therefore we demand +1 for PROP,
            // therefore having TSEG1 min in DBT = 3 TQ!
            test_data_bit_timing.prop_ = elem_test.index_ + 1;
            test_data_bit_timing.ph2_ = nbt.GetBitLenTQ() - elem_test.index_;

            /* Re-configure bit-timing for this test so that frames are generated with it! */
            this->nbt = test_nom_bit_timing;
            this->dbt = test_data_bit_timing;

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(test_nom_bit_timing, dbt);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            test_nom_bit_timing.Print();
            TestMessage("Data bit timing for this elementary test:");
            test_data_bit_timing.Print();

            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, BrsFlag::DoShift,
                                                       EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Force SYNC + Prop + Ph1 - 1 starting time quantas of BRS to Dominant.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *brs_bit = drv_bit_frm->GetBitOf(0, BitKind::Brs);
            size_t num_time_quantas = nbt.prop_ + nbt.ph1_;
            for (size_t i = 0; i < num_time_quantas; i++)
                brs_bit->ForceTQ(i, BitVal::Dominant);

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