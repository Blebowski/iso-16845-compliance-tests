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
 * @test ISO16845 8.2.5
 *
 * @brief This test verifies that the IUT detects a form error when the
 *        transmitted fixed-form bit field is different from the bit it
 *        receives.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter, ACK Delimiter, EOF (first, fourth and last one), FDF = 0
 *
 *  CAN FD Enabled
 *      CRC Delimiter, ACK Delimiter (2 ACK bits used),EOF (first, fourth and
 *      last one), Fixed Stuff bit at CRC17, Fixed Stuff bit at CRC21, FDF = 1
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      There are five elementary tests to perform.
 *          #1  CRC Delimiter
 *          #2  ACK Delimiter
 *          #3  EOF bit 1
 *          #4  EOF bit 4
 *          #5  EOF bit 7
 *
 *  CAN FD enabled
 *      There are 16 elementary tests to perform:
 *          #1  CRC Delimiter
 *          #2  ACK Delimiter
 *          #3  EOF bit 1
 *          #4  EOF bit 4
 *          #5  EOF bit 7
 *      #6-#11  Fixed stuff bit at CRC(17) - (6 bits)
 *      #7-#18  Fixed stuff bit at CRC(21) - (7 bits)
 *
 *  Note: This numbering is slightly different that in ISO, but it should be OK
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT creates a form
 *  error on the fields listed in elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame at the bit position following the
 *  corrupted bit.
 *  The IUT shall restart the transmission of the frame as soon as the bus is
 *  idle.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_2_5 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (int i = 0; i < 5; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
            for (int i = 0; i < 18; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            SetupMonitorTxTests();
            /* TX to RX feedback must be disabled since we corrupt dominant bits to Recessive */
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* Choose DLC based on elementary test */
            uint8_t dlc;
            switch (elem_test.index_)
            {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                dlc = rand() % 9;
                break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                dlc = 0xA; // Should cause CRC17
                break;
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
                dlc = 0xD; // Should cause CRC21
                break;
            }

            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, BrsFlag::DoShift,
                                                       EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* Second frame the same due to retransmission. */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert dominant ACK so that IUT does not detect ACK Error!
             *   2. Flip bit as given by elementary test cases.
             *   3. Insert Active error frame from next bit to both driven and monitored frames
             *      (TX/RX feedback disabled)
             *   4. Append retransmitted frame
             *************************************************************************************/
            drv_bit_frm->PutAck(dut_input_delay);

            Bit *bit_to_corrupt;
            switch (elem_test.index_)
            {
            case 1:
                bit_to_corrupt = drv_bit_frm->GetBitOf(0, BitKind::CrcDelim);
                break;
            case 2:
                bit_to_corrupt = drv_bit_frm->GetBitOf(0, BitKind::AckDelim);
                break;
            case 3:
                bit_to_corrupt = drv_bit_frm->GetBitOf(0, BitKind::Eof);
                break;
            case 4:
                bit_to_corrupt = drv_bit_frm->GetBitOf(3, BitKind::Eof);
                break;
            case 5:
                bit_to_corrupt = drv_bit_frm->GetBitOf(6, BitKind::Eof);
                break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                bit_to_corrupt = drv_bit_frm->GetFixedStuffBit(elem_test.index_ - 6);
                break;
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
                bit_to_corrupt = drv_bit_frm->GetFixedStuffBit(elem_test.index_ - 12);
                break;
            default:
                TestMessage("Invalid Elementary test index: %d", elem_test.index_);
                bit_to_corrupt = drv_bit_frm->GetFixedStuffBit(elem_test.index_ - 12);
            }

            int bit_index = drv_bit_frm->GetBitIndex(bit_to_corrupt);
            drv_bit_frm->FlipBitAndCompensate(bit_to_corrupt, dut_input_delay);

            drv_bit_frm->InsertActErrFrm(bit_index + 1);
            mon_bit_frm->InsertActErrFrm(bit_index + 1);

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetTec(0); /* Avoid turning error passive */
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();

            return FinishElemTest();
        }

};