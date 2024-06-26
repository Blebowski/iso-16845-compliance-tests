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
 * @date 11.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.2.6
 *
 * @brief The purpose of this test is to verify that an IUT detecting a CRC
 *        error and a form error on the CRC delimiter in the same frame
 *        generates only one single 6 bits long error flag starting on the bit
 *        following the CRC delimiter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter, FDF = 0
 *
 *  CAN FD Enabled
 *      CRC, DLC - to cause different CRC types. FDF = 1
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      #1 CRC (15)
 *
 *  CAN FD Enabled
 *      #1 DLC ≤ 10 − > CRC (17)
 *      #2 DLC > 10 − > CRC (21)
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with CRC error and form error at CRC delimiter
 *  according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate one active error frame starting at the bit position
 *  following the CRC delimiter.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_2_6: public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));
            AddElemTest(TestVariant::CanFdEna, ElemTest(2, FrameKind::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc;
            if (test_variant == TestVariant::Common)
            {
                dlc = static_cast<uint8_t>(rand() % 9);
            }
            else if (elem_test.index_ == 1)
            {
                if (rand() % 2)
                    dlc = 0x9;
                else
                    dlc = 0xA;
            } else
            {
                dlc = static_cast<uint8_t>((rand() % 5) + 11);
            }

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force random CRC bit to its opposite value
             *   3. Force CRC Delimiter to dominant.
             *   4. Compensate length of CRC field in the monitored frame. Since the driven frame
             *      gets a bit flipped, its original CRC length may have changed. Thus, in the
             *      monitored frame, we need to make it equal to driven frame, so that LT does
             *      not expect error frame at wrong position!
             *   5. Insert Error frame to position of ACK!
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            do {
                Bit *bit = drv_bit_frm->GetRandBitOf(BitKind::Crc);
                size_t index = drv_bit_frm->GetBitIndex(bit);

                // Ignore stuff bits, and bits just before stuff bits. If we flip bit before
                // stuff bit, then we cause stuff error too!
                if (bit->stuff_kind_ == StuffKind::NoStuff &&
                    drv_bit_frm->GetBit(index + 1)->stuff_kind_ == StuffKind::NoStuff)
                    {
                        // This should cause only CRC error, no stuff error!
                        bit->FlipVal();
                        break;
                    }
            } while (true);

            // If we are in CAN 2.0, then flipping also non-stuff bit can cause change of
            // CRC length since number of stuff bits can change! Therefore, we need to recalculate
            // stuff bits, but keep the CRC (since it contains corrupted bit that we rely on)!
            drv_bit_frm->UpdateFrame(false);

            drv_bit_frm->GetBitOf(0, BitKind::CrcDelim)->val_ = BitVal::Dominant;

            // Compensate CRC length in monitored frame ot match length in the driven frame.
            // Needed because flipping a bit in driven frame may have changed CRC lenght.
            TestMessage("CRC field length in driven frame: %zu"    , drv_bit_frm->GetFieldLen(BitKind::Crc));
            TestMessage("CRC field length in monitored frame: %zu" , mon_bit_frm->GetFieldLen(BitKind::Crc));

            while (mon_bit_frm->GetFieldLen(BitKind::Crc) <
                   drv_bit_frm->GetFieldLen(BitKind::Crc)) {
                size_t index = mon_bit_frm->GetBitIndex(mon_bit_frm->GetBitOf(0, BitKind::Crc));
                mon_bit_frm->InsertBit(BitKind::Crc, BitVal::Recessive, index);
                TestMessage("Applying CRC length compenstaion, lengthening CRC by 1 recessive bit...");
            }

            while (mon_bit_frm->GetFieldLen(BitKind::Crc) >
                   drv_bit_frm->GetFieldLen(BitKind::Crc)) {
                mon_bit_frm->RemoveBit(mon_bit_frm->GetBitOf(0, BitKind::Crc));
                TestMessage("Applying CRC length compenstaion, shortening CRC by 1 recessive bit...");
            }

            mon_bit_frm->InsertActErrFrm(0, BitKind::Ack);
            drv_bit_frm->InsertActErrFrm(0, BitKind::Ack);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};