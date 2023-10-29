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
 * @date 17.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.2.4
 *
 * @brief This test verifies that the IUT detects a stuff error whenever it
 *        receives 6 consecutive bits of the same value until the position of
 *        the CRC delimiter in a base format frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *      CAN FD Enabled:
 *          DATA byte 0–63
 *          ID = 555 h
 *          IDE = 0
 *          DLC = 15
 *          FDF = 1
 *
 * Elementary test cases:
 *  All 1 008 stuff bits within the defined data bytes 1 to 63 will be tested.
 *
 *              Data Byte 0               Data bytes 1 – 63
 *      #1          0x10                        0x78
 *      #2          0x78                        0x3C
 *      #3          0x34                        0x1E
 *      #4          0x12                        0x0F
 *      #5          0x0F                        0x87
 *      #6          0x17                        0xC3
 *      #7          0x43                        0xE1
 *      #8          0x21                        0xF0
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for each elementary test. In each elementary
 *  test, the LT forces one of the stuff bits to its complement.
 *
 * Response:
 *  The IUT shall generate an active error frame starting at the bit position
 *  following the stuff error.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_2_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (int i = 0; i < 8; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data[64] = {};

            switch (elem_test.index_)
            {
            case 1:
                data[0] = 0x10;
                for (int i = 1; i < 64; i++)
                    data[i] = 0x78;
                break;

            case 2:
                data[0] = 0x78;
                for (int i = 1; i < 64; i++)
                    data[i] = 0x3C;
                break;

            case 3:
                data[0] = 0x34;
                for (int i = 1; i < 64; i++)
                    data[i] = 0x1E;
                break;

            case 4:
                data[0] = 0x12;
                for (int i = 1; i < 64; i++)
                    data[i] = 0x0F;
                break;

            case 5:
                data[0] = 0x0F;
                for (int i = 1; i < 64; i++)
                    data[i] = 0x87;
                break;

            case 6:
                data[0] = 0x17;
                for (int i = 1; i < 64; i++)
                    data[i] = 0xC3;
                break;

            case 7:
                data[0] = 0x43;
                for (int i = 1; i < 64; i++)
                    data[i] = 0xE1;
                break;

            case 8:
                data[0] = 0x21;
                for (int i = 1; i < 64; i++)
                    data[i] = 0xF0;
                break;

            default:
                break;
            }

            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, IdentKind::Base,
                                    RtrFlag::Data, BrsFlag::DoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0xF, 0x555, data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitored frame to received.
             *   2. Pick one of the stuff bits within the frame and flip its value.
             *   3. Insert Active Error frame to monitored frame. Insert Passive Error frame
             *      to driven frame (TX/RX feedback enabled).
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            int num_stuff_bits = drv_bit_frm->GetNumStuffBits(StuffKind::Normal);

            /* In FD enabled variant, if last bit of data field is stuff bit, but model has this bit
             * as fixed stuff bit before Stuff count. So count in also each fixed stuff bit even
             * if last bit of data is NOT regular stuff bit. Then total number of stuff bits within
             * FD enabled variant will be higher than in ISO 16845, but this does not mind!
             */
            Bit *bit = drv_bit_frm->GetBitOf(0, BitKind::StuffCnt);
            int index = drv_bit_frm->GetBitIndex(bit);
            BitVal value = drv_bit_frm->GetBit(index - 1)->val_;
            if ((value == drv_bit_frm->GetBit(index - 2)->val_) &&
                (value == drv_bit_frm->GetBit(index - 3)->val_) &&
                (value == drv_bit_frm->GetBit(index - 4)->val_) &&
                (value == drv_bit_frm->GetBit(index - 5)->val_))
                num_stuff_bits++;

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            for (int stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
            {
                TestMessage("Testing stuff bit nr: %d", stuff_bit);
                stuff_bits_in_variant++;

                /*
                 * Copy frame to second frame so that we dont loose modification of bits.
                 * Corrupt only second one.
                 */
                drv_bit_frm_2 = std::make_unique<BitFrame>(*drv_bit_frm);
                mon_bit_frm_2 = std::make_unique<BitFrame>(*mon_bit_frm);

                Bit *stuff_bit_to_flip = drv_bit_frm_2->GetStuffBit(stuff_bit);
                int bit_index = drv_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                stuff_bit_to_flip->FlipVal();

                drv_bit_frm_2->InsertPasErrFrm(bit_index + 1);
                mon_bit_frm_2->InsertActErrFrm(bit_index + 1);

                /* Do the test itself */
                dut_ifc->SetRec(0);
                PushFramesToLT(*drv_bit_frm_2, *mon_bit_frm_2);
                RunLT(true, true);
                CheckLTResult();

                drv_bit_frm_2.reset();
                mon_bit_frm_2.reset();
            }
            FreeTestObjects();

            return FinishElemTest();
        }
};
