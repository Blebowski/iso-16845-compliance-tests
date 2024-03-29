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
 * @date 17.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.7.7
 *
 * @brief The purpose of this test is to verify that an IUT transmitting a
 *        dominant bit does not perform any resynchronization as a result of a
 *        recessive to dominant edge with a positive phase error.
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
 *      #1 The LT delays each recessive to dominant edge by 2 time quanta.
 *
 *  Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  While the IUT is transmitting the frame, the LT delays each recessive to
 *  dominant according to elementary test cases.
 *
 * Response:
 *  The IUT shall continue transmitting frame without any resynchronization.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_7 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTestForEachSP(TestVariant::Common, true, FrameKind::Can20);

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Minimal lenght of PH1/PROP must be such that the delayed edge by two time
            // quanta + input delay of IUT will still guarantee that proper value of
            // transmitted bit will propagate to IUTs reception in time!
            // For CTU CAN FD, this is:
            //      input delay (2) + delay of edge due to test (2) + 1 = 5. Minimal
            //      duration of TSEG1 = 5 cycles!!!
            nbt = GenerateSPForTest(elem_test, true, 4);

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

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Search through CAN frame and search for each dominant bit following recessive
             *      bit. For each such bit, force its first two time quantas to recessive. This
             *      delays the synchronization edge by two time quantas.
             *
             * Note: TX/RX feedback must be disabled, since we modify driven frame.
             * Note: The overall lenght of each bit is kept! Therefore, if IUT will  synchronize,
             *       then it will be off from monitored frame whose bits are not prolonged/shorte-
             *       ned in any way! So monitoring frame succesfully, checks that no synchroniza-
             *       tion has been done! This behavior has been checked with bug inserted into IUT
             *       and the test really failed!
             *************************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            // I know this search is performance suicide, but again, we dont give a shit
            // about performance here... It is just a model running on powerfull PC!
            for (size_t i = 0; i < drv_bit_frm->GetLen() - 1; i++)
            {
                Bit *curr = drv_bit_frm->GetBit(i);
                Bit *next = drv_bit_frm->GetBit(i + 1);

                if (curr->val_ == BitVal::Recessive &&
                    next->val_ == BitVal::Dominant)
                {
                    next->GetTQ(0)->ForceVal(BitVal::Recessive);
                    next->GetTQ(1)->ForceVal(BitVal::Recessive);
                }
            }

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