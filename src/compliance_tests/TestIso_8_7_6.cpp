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
 * @test ISO16845 8.7.6
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there are two recessive to
 *        dominant edges between two sample points where the first edge comes
 *        before the synchronization segment.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 *
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 *
 * Elementary test cases:
 *  There is one elementary test to perform for each programmable sampling
 *  point inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 Recessive glitch at 2nd TQ in early started dominant bit.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  A recessive bit which is followed by a dominant bit is shortened by one
 *  time quantum.
 *  After one time quantum of dominant value, the LT forces one time quantum
 *  to recessive value.
 *
 * Response:
 *  The IUT shall send a dominant to recessive edge an integer number of bit
 *  times after the first recessive to dominant edge applied by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_6 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTestForEachSP(TestVariant::Common, true, FrameKind::Can20);

            SetupMonitorTxTests();

            TEST_ASSERT(nbt.brp_ > 2,
                        "BRP Nominal must be bigger than 2 in this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            nbt = GenerateSPForTest(elem_test, true, 2);

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(nbt, dbt);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            nbt.Print();

            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /******************************************************************************
             * Modify test frames:
             *   1. Set Ack dominant in driven frame.
             *   2. Choose random recessive bit which is followed by dominant bit.
             *   3. Shorten chosen bit by 1 TQ in driven frame. This will cause negative
             *      resynchronization with e = -1. IUT will finish TSEG1 immediately and
             *      have TSEG2 of next bit shorter by -1. So shorten Sync of next bit
             *      in monitored frame by 1.
             *   4. Force 2nd Time quanta of dominant bit after the random recessive bit
             *      to Recessive in driven frame.
             *
             *   Note: The check that next edge shall be sent integer number of time
             *         quantas after it is executed by the fact that we don not do any
             *         further resynchronizations. Model takes care of it then!
             *****************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            Bit *rand_bit = nullptr;
            Bit *next_bit = nullptr;
            size_t rand_bit_index;
            do {
                rand_bit = drv_bit_frm->GetRandBit(BitVal::Recessive);
                rand_bit_index = drv_bit_frm->GetBitIndex(rand_bit);
                if (rand_bit_index == drv_bit_frm->GetLen() - 1)
                    continue;
                next_bit = drv_bit_frm->GetBit(rand_bit_index + 1);
            } while (next_bit == nullptr || next_bit->val_ == BitVal::Recessive);

            rand_bit->ShortenPhase(BitPhase::Ph2, 1);
            mon_bit_frm->GetBit(rand_bit_index + 1)->ShortenPhase(BitPhase::Sync, 1);

            next_bit->ForceTQ(1, BitVal::Recessive);

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