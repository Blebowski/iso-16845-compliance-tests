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
 * @date 18.12.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.2.5
 *
 * @brief The purpose of this test is to verify:
 *          — that the IUT uses the specific CRC mechanism according to frame
 *            format,
 *          — that the IUT detecting a CRC error and generates an error frame
 *            at the correct position, and
 *          — that the IUT does not detect an error when monitoring a dominant
 *            bit at the ACK slot while sending a recessive one.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *          CRC, FDF = 0, SOF
 *
 * Elementary test cases:
 *    Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      Number of elementary tests: 3
 *      #1 A dominant bit in the CRC field is changed in a recessive bit.
 *      #2 A recessive bit in the CRC field is changed in a dominant bit.
 *      #3 The dominant SOF bit in the frame is changed in a recessive
 *         one followed by an ID 001 h.
 *
 *    CAN FD Enabled:
 *      #1 and #2 A dominant bit in the CRC field is changed in a recessive
 *         bit for CRC-17 with DLC ≤ 10 (#1) and CRC-21 with DLC > 10 (#2)
 *         (test for CRC value).
 *
 *      #3 and #4 A recessive bit in the CRC field is changed in a dominant
 *         bit for CRC-17 with DLC ≤ 10 (#3) and CRC-21 with DLC > 10 (#4)
 *         (test for CRC value).
 *
 *      #5 The test system sends a frame where two times a recessive stuff
 *         bit becomes a normal bit by losing one of the previous bits by
 *         synchronization issues while the CRC register is equal zero
 *         (test for stuff-counter).
 *
 *      #6 The parity bit of the stuff count and the following fixed stuff
 *         bit changed into their opposite values (test for stuff-counter
 *         parity bit value).
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for each elementary test. The LT modifies the
 *  frame according to elementary test cases.
 *
 * Response:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      The IUT shall not acknowledge the test frame. The IUT shall generate
 *      an active error frame starting at the first bit position following
 *      the ACK delimiter.
 *
 *  CAN FD Enabled:
 *      The IUT shall not acknowledge the test frame. The IUT shall generate
 *      an active error frame starting at the fourth bit position following the
 *      CRC delimiter.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_2_5 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t j = 0; j < 3; j++)
                AddElemTest(TestVariant::Common, ElemTest(j + 1, FrameKind::Can20));
            for (size_t j = 0; j < 6; j++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(j + 1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        /*
         * Choose CRC bit with required value for CRC error insertion.
         * Following bits are left out:
         *  - Stuff bit
         *  - A bit before stuff bit (if not left out, undesired stuff error will occur)
         */
        int ChooseCrcBitToCorrupt(BitFrame *bit_frame, BitVal bit_value)
        {
            Bit *bit;
            bool is_ok;
            do {
                is_ok = true;
                bit = bit_frame->GetRandBitOf(BitKind::Crc);
                int bit_index = bit_frame->GetBitIndex(bit);

                if (bit->stuff_kind_ != StuffKind::NoStuff)
                    is_ok = false;
                if (drv_bit_frm->GetBit(bit_index + 1)->stuff_kind_ != StuffKind::NoStuff)
                    is_ok = false;
                if (bit->val_ != bit_value)
                    is_ok = false;
            } while (!is_ok);
            return bit_frame->GetBitIndex(bit);
        }

        /*
         * Inserts CRC error on bit index position. Flips the bit and updates the frame,
         * so that CRC lenght will be correct as when IUT will receive such sequence!
         */
        void InsertCrcError(int index)
        {
            Bit *drv_bit = drv_bit_frm->GetBit(index);
            Bit *mon_bit = mon_bit_frm->GetBit(index);

            drv_bit->FlipVal();
            mon_bit->FlipVal();

            drv_bit_frm->UpdateFrame(false);
            mon_bit_frm->UpdateFrame(false);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int id = rand() % (int)pow(2, 11);
            uint8_t dlc = 0x0;

            if (test_variant == TestVariant::Common) {
                if (elem_test.index_ == 3) {
                    id = 0x1;
                }
            } else if (test_variant == TestVariant::CanFdEna) {
                if (elem_test.index_ == 1 || elem_test.index_ == 3) {
                    dlc = rand() % 11; // To cause CRC 17
                } else if (elem_test.index_ == 2 || elem_test.index_ == 4) {
                    dlc = rand() % 5 + 11; // To cause CRC 21
                } else {
                    dlc = rand() % 15;
                }
            }

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, id);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Modify bit as given by elementary test. Re-stuff CRC since lenght of
             *      CRC might have changed due to stuff error. This is needed since, flipped CRC
             *      bit might have caused change of CRC length due to stuff bits!
             *   2. Turn monitored frame as received. Force ACK low, since IUT shall not
             *      acknowledge the frame.
             *   3. Insert Active error frame to monitored frame after ACK delimiter. This covers
             *      also CAN FD enabled option, since model contains 2 bits for ACK in FD frame.
             *      Insert Passive Error frame to driven frame (TX/RX feedback enabled).
             *************************************************************************************/

            if (test_variant == TestVariant::Common) {
                if (elem_test.index_ == 1 || elem_test.index_ == 2) {
                    BitVal bit_value;
                    if (elem_test.index_ == 1) {
                        bit_value = BitVal::Dominant;
                    } else {
                        bit_value = BitVal::Recessive;
                    }

                    InsertCrcError(ChooseCrcBitToCorrupt(drv_bit_frm.get(), bit_value));

                } else if (elem_test.index_ == 3) {
                    drv_bit_frm->GetBitOf(0, BitKind::Sof)->val_ = BitVal::Recessive;
                }

            } else if (test_variant == TestVariant::CanFdEna) {
                if (elem_test.index_ >= 1 && elem_test.index_ <= 4) {
                    BitVal bit_value;
                    if (elem_test.index_ == 1 || elem_test.index_ == 3) {
                        bit_value = BitVal::Dominant;
                    } else {
                        bit_value = BitVal::Recessive;
                    }

                    InsertCrcError(ChooseCrcBitToCorrupt(drv_bit_frm.get(), bit_value));

                } else if (elem_test.index_ == 5) {
                    // TODO: Do the shortening to test CRC Issue!

                } else if (elem_test.index_ == 6) {
                    drv_bit_frm->GetBitOf(0, BitKind::StuffParity)->FlipVal();
                    // Stuff bit post stuff parity is inserted with same field type!
                    drv_bit_frm->GetBitOf(1, BitKind::StuffParity)->FlipVal();
                }
            }

            mon_bit_frm->ConvRXFrame();
            mon_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            mon_bit_frm->InsertActErrFrm(0, BitKind::Eof);
            drv_bit_frm->InsertPasErrFrm(0, BitKind::Eof);

            // TODO: Skip test for CRC issue, will be finished later!
            if (elem_test.index_ == 5)
                return FinishElemTest();

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckNoRxFrame();

            FreeTestObjects();
            return FinishElemTest();
        }

};
