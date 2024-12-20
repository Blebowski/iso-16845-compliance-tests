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
 * @date 21.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.6
 *
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting a bit error in a data frame on one of the
 *        following fields described in test variables.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      FDF = 0
 *
 *  CAN FD Enabled
 *      FDF = 1
 *
 * Elementary test cases:
 *   Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *     There are eight elementary tests to perform.
 *     In the arbitration field, only bit error on dominant bits shall be
 *     considered.
 *          #1 SOF
 *          #2 dominant ID
 *          #3 dominant DLC
 *          #4 recessive DLC
 *          #5 dominant DATA
 *          #6 recessive DATA
 *          #7 dominant CRC
 *          #8 recessive CRC
 *
 *   CAN FD Enabled
 *      There are 10 elementary tests to perform.
 *      In the arbitration field, only bit error on dominant bits shall be
 *      considered.
 *          #1 SOF
 *          #2 dominant ID
 *          #3 dominant DLC
 *          #4 recessive DLC
 *          #5 dominant DATA
 *          #6 recessive DATA
 *          #7 dominant CRC (17)
 *          #8 recessive CRC (17)
 *          #9 dominant CRC (21)
 *          #10 recessive CRC (21)
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT corrupts a bit according to elementary test cases.
 *
 * Response:
 *  The IUT’s TEC value shall be increased by 8 at the bit error detection.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <math.h>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_6 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 8; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
            for (size_t i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            do
            {
                TestBigMessage("Generating random frame...");
                frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, RtrFlag::Data,
                                                           EsiFlag::ErrAct);
                uint8_t dlc;

                if (test_variant == TestVariant::CanFdEna)
                {
                    /* To achieve CRC17 or CRC21 in elem tests 7-10 of Can FD Enabled variant */
                    if (elem_test.index_ == 7 || elem_test.index_ == 8)
                        dlc = static_cast<uint8_t>((rand() % 0xA) + 1);
                    else if (elem_test.index_ == 9 || elem_test.index_ == 10)
                        dlc = static_cast<uint8_t>((rand() % 5) + 0xB);
                    else
                        dlc = static_cast<uint8_t>(rand() % 0xF);
                    } else {
                        dlc = static_cast<uint8_t>(rand() % 8 + 1);
                    }
                    gold_frm = std::make_unique<Frame>(*frm_flags, dlc);
                    RandomizeAndPrint(gold_frm.get());

            // We repeat generating random frame as long as we have any of the fields
            // whose non-stuff bits we could possibly flip with all zeroes or all ones!
            } while (gold_frm->identifier() == (CAN_BASE_ID_MAX - 1) ||
                     gold_frm->identifier() == (CAN_EXTENDED_ID_MAX - 1) ||
                     gold_frm->dlc() == 0x0 ||
                     gold_frm->dlc() == 0xF ||
                     gold_frm->data(0) == 0x00 ||
                     gold_frm->data(0) == 0xFF);

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Corrupt bit as given by elementary test case. Avoid corrupting stuff bits
             *      alltogether!
             *   2. Insert active error frame to both driven and monitored frames from next bit on.
             *   3. Append the same frame again with ACK on driven frame. This emulates retransmi-
             *      tted frame by IUT.
             *************************************************************************************/
            BitKind bit_type_to_corrupt = BitKind::Sof;
            BitVal value_to_corrupt = BitVal::Dominant;
            switch (elem_test.index_)
            {
            case 1:
                bit_type_to_corrupt = BitKind::Sof;
                break;
            case 2:
                bit_type_to_corrupt = BitKind::BaseIdent;
                break;
            case 3:
                bit_type_to_corrupt = BitKind::Dlc;
                break;
            case 4:
                bit_type_to_corrupt = BitKind::Dlc;
                value_to_corrupt = BitVal::Recessive;
                break;
            case 5:
                bit_type_to_corrupt = BitKind::Data;
                break;
            case 6:
                bit_type_to_corrupt = BitKind::Data;
                value_to_corrupt = BitVal::Recessive;
                break;
            case 7:
            case 9:
                bit_type_to_corrupt = BitKind::Crc;
                break;
            case 8:
            case 10:
                bit_type_to_corrupt = BitKind::Crc;
                value_to_corrupt = BitVal::Recessive;
                break;
            default:
                break;
            }

            /* Find random bit that:
                - Belongs to the bitfield based on elementary test
                - Is of requested value
                - Is non-stuff bit

               The frame was previously generated such that all fields but CRC
               will have at least one Dominant and one Recessive bit.

               It may happen all zero CRC will occur. So for CRC field allow flipping
               also stuff bits. In such case it is simultaneous stuff error and
               bit error, but bit error shall have priority. Therefore DUTs TEC
               shall also increment by 8.
             */

            Bit *bit_to_corrupt;
            bool keep_looping = true;

            do {
                bit_to_corrupt = drv_bit_frm->GetRandBitOf(bit_type_to_corrupt);

                if (bit_to_corrupt->val_ == value_to_corrupt) {
                    if (bit_to_corrupt->stuff_kind_ == StuffKind::NoStuff)
                        keep_looping = false;

                    // Special case to handle potentially all zero CRC, allow flipping
                    // also stuff bits
                    if (bit_type_to_corrupt == BitKind::Crc)
                        keep_looping = false;
                }
            } while (keep_looping);


            drv_bit_frm->FlipBitAndCompensate(bit_to_corrupt, dut_input_delay);
            size_t bit_index = drv_bit_frm->GetBitIndex(bit_to_corrupt);

            drv_bit_frm->InsertActErrFrm(bit_index + 1);
            mon_bit_frm->InsertActErrFrm(bit_index + 1);

            drv_bit_frm_2->PutAck(dut_input_delay);

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            if (dut_ifc->GetTec() > 100)
                dut_ifc->SetTec(0);

            tec_old = dut_ifc->GetTec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();
            /* +8 for error, -1 for succsefull retransmission */
            CheckTecChange(tec_old, 7);

            return FinishElemTest();
        }

};