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
 * @date 30.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.2.8
 *
 * @brief This test verifies that the IUT detects an error when after the
 *        transmission of 5 identical bits, it receives a sixth bit identical
 *        to the five precedents.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  CAN FD Enabled
 *      Data byte 0 - 63, ID = 0x555, IDE = 0, DLC = 15, FDF = 1
 *
 * Elementary test cases:
 *  CAN FD Enabled
 *   All 1 008 stuff bit positions within the defined data bytes will be tested.
 *
 *   There are 35 elementary tests to perform.
 *
 *                      Data byte 0                   Data bytes 1 - 63
 *      #1 to #126          0x10                            0x78
 *    #127 to #252          0x78                            0x3C
 *    #253 to #378          0x34                            0x1E
 *    #379 to #504          0x12                            0x0F
 *    #505 to #630          0x0F                            0x87
 *    #631 to #756          0x17                            0xC3
 *    #757 to #882          0x43                            0xE1
 *    #883 to #1008         0x21                            0xF0
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for each elementary test. In each elementary
 *  test, the LT forces another one of the stuff bits to its complement.
 *
 * Response:
 *  The IUT shall generate an active error frame starting at the bit position
 *  following the bit error at stuff bit position.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_2_8 : public test::TestBase
{
    public:
        bool one_shot_enabled;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (size_t i = 0; i < 1008; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            SetupMonitorTxTests();
            /* TX to RX feedback must be disabled since we corrupt dominant bits to Recessive */
            one_shot_enabled = dut_ifc->ConfigureOneShot(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data[64] = {};
            uint8_t data_first;
            uint8_t data_rest;

            size_t stuff_bit_index;

            if (elem_test.index_ < 127) {
                data_first = 0x10;
                data_rest = 0x78;
                stuff_bit_index = elem_test.index_ - 1;
            } else if (elem_test.index_ < 253){
                data_first = 0x78;
                data_rest = 0x3C;
                stuff_bit_index = elem_test.index_ - 127;
            } else if (elem_test.index_ < 379){
                data_first = 0x34;
                data_rest = 0x1E;
                stuff_bit_index = elem_test.index_ - 253;
            } else if (elem_test.index_ < 505){
                data_first = 0x12;
                data_rest = 0x0F;
                stuff_bit_index = elem_test.index_ - 379;
            } else if (elem_test.index_ < 631){
                data_first = 0x0F;
                data_rest = 0x87;
                stuff_bit_index = elem_test.index_ - 505;
            } else if (elem_test.index_ < 757){
                data_first = 0x17;
                data_rest = 0xC3;
                stuff_bit_index = elem_test.index_ - 631;
            } else if (elem_test.index_ < 883){
                data_first = 0x43;
                data_rest = 0xE1;
                stuff_bit_index = elem_test.index_ - 757;
            } else {
                stuff_bit_index = elem_test.index_ - 883;
                data_first = 0x21;
                data_rest = 0xF0;
            }

            data[0] = data_first;
            for (size_t i = 1; i < 64; i++)
                data[i] = data_rest;

            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, IdentKind::Base,
                                        RtrFlag::Data, BrsFlag::DoShift, EsiFlag::ErrAct);

            gold_frm = std::make_unique<Frame>(*frm_flags, 0xF, 0x555, data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Choose stuff bit as given by elementary test. Description of elementary tests
             *      should match number of stuff bits (e.g. in first frame 126 stuff bits)!
             *   2. Corrupt stuff bit from point 1 to opposite value.
             *   3. Insert Active Error frame from next bit on.
             *   4. Append retransmitted frame if one shot mode is not enabled. If it is enabled,
             *      IUT will not retransmitt the frame. This serves to shorten the test time!
             *************************************************************************************/
            size_t num_stuff_bits = drv_bit_frm->GetNumStuffBits(BitKind::Data, StuffKind::Normal);
            Bit *stuff_bit;

            /* It can be that last bit is right after last bit of data!! */
            if (num_stuff_bits > stuff_bit_index)
                stuff_bit = drv_bit_frm->GetStuffBit(stuff_bit_index, BitKind::Data);
            else
                stuff_bit = drv_bit_frm->GetStuffBit(0, BitKind::StuffCnt);

            stuff_bit->FlipVal();
            size_t bit_index = drv_bit_frm->GetBitIndex(stuff_bit);

            drv_bit_frm->InsertActErrFrm(bit_index + 1);
            mon_bit_frm->InsertActErrFrm(bit_index + 1);

            if (!one_shot_enabled)
            {
                drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
                drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
                mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());
            }

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             ****************************************************************************/
            dut_ifc->SetTec(0);
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            drv_bit_frm.reset();
            mon_bit_frm.reset();
            drv_bit_frm_2.reset();
            mon_bit_frm_2.reset();

            return FinishElemTest();
        }

};