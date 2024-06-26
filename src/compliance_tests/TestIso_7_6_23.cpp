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
 * @date 22.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.23
 *
 * @brief The purpose of this test is to verify that the IUT switches to
 *        protocol exception on non-nominal values of the bits described in
 *        test variables and did not change the CAN error counter.
 * @version CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  CAN FD Tolerant:
 *      FDF = 1
 *      DLC
 *      Data: All data byte with the same value
 *      Bit rate ratio between nominal and data bit rate
 *
 *  CAN FD enabled:
 *      FDF = 1
 *      “res” bit = 1
 *      DLC
 *      Data: All data byte with the same value
 *      Bit rate ratio between nominal and data bit rate
 *
 * Elementary test cases:
 *   CAN FD Tolerant:
 *      Test    Format      DLC         Data        Bit rate ratio
 *       #1      FBFF       0xA         0xAA            1:2
 *       #2      FBFF       0xF         0xFF            1:8
 *       #3      CBFF       0xF         0xFF             -
 *
 *   CAN FD Enabled:
 *       #1      FBFF       0xA         0xAA            1:2
 *       #2      FBFF       0xF         0xFF            1:8
 *       #3      CBFF       0xF         0xFF             -
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  a) The test system causes a receive error to initialize the REC value to 9.
 *  b) A single test frame is used for the elementary test, followed immediately
 *     by a valid Classical CAN frame.
 *
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall not acknowledge the test frame.
 *  A following data frame in classical frame format received by the IUT during
 *  the test state shall match the data sent in the test frame.
 *  The IUT’s REC value shall be 8 after reception of the valid Classical
 *  CAN frame.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_23 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::FdTolAndFdEna);

            size_t num_elem_tests;
            if (test_variants[0] == TestVariant::CanFdTol)
                num_elem_tests = 3;
            else if (test_variants[0] == TestVariant::CanFdEna)
                num_elem_tests = 2;
            else
                num_elem_tests = 0;

            for (size_t i = 0; i < num_elem_tests; i++)
                if (i < 3)
                    elem_tests[0].push_back(ElemTest(i + 1, FrameKind::CanFd));
                else
                    elem_tests[0].push_back(ElemTest(i + 1, FrameKind::Can20));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /******************************************************************************
             * First configure bit rate. Take configured bit rate for CAN FD and multiply
             * by 8/4 to get data bit rate. This-way we hope not to get out of DUTs spec
             * for bit timing!
             *****************************************************************************/
            dut_ifc->Disable();
            dut_ifc->ConfigureProtocolException(true);
            nbt = dbt;
            if (elem_test.index_ == 1)
                nbt.brp_ = dbt.brp_ * 2;
            else
                nbt.brp_ = dbt.brp_ * 8;
            dut_ifc->ConfigureBitTiming(nbt, dbt);

            /* Enable and wait till integration is over again */
            dut_ifc->Enable();
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(2000);

            /******************************************************************************
             * Generate frames!
             *****************************************************************************/
            // Approriate Frame type is generated in ConfigureTest!
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_);

            if (elem_test.index_ == 1)
            {
                uint8_t data[64] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
                gold_frm = std::make_unique<Frame>(*frm_flags, 0xA, data);
            } else {
                uint8_t data[64] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                gold_frm = std::make_unique<Frame>(*frm_flags, 0xF, data);
            }
            RandomizeAndPrint(gold_frm.get());

            frm_flags_2 = std::make_unique<FrameFlags>(FrameKind::Can20);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**********************************************************************************
             * Modify test frames:
             *   1. Modify test frame according to elementary test cases. FD Tolerant variant
             *      needs no modifications since FDF recessive is enough to trigger protocol
             *      exception! FD Enabled needs bit after FDF forced recessive!
             *   2. Update the frames since this might have changed CRC/length.
             *   3. Turn monitored frame as if received!
             *   4. Remove ACK from monitored frame (since IUT is in protocol exception). No
             *      other modifications are needed since if monitored frame is as if received,
             *      IUT transmitts all recessive! IUT should be now monitoring until it
             *      receives 11 consecutive recessive bits!
             *   5. Append second frame directly after first frame as if transmitted by LT.
             **********************************************************************************/
            if (test_variant == TestVariant::CanFdEna)
            {
                drv_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
                mon_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
            }

            drv_bit_frm->UpdateFrame();
            mon_bit_frm->UpdateFrame();

            mon_bit_frm->ConvRXFrame();

            mon_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2->ConvRXFrame();

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            /**********************************************************************************
             * Execute test
             *********************************************************************************/
            dut_ifc->SetRec(9);
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm_2);
            CheckRecChange(rec_old, -1);

            return FinishElemTest();
        }
};