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
 * @date 29.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.2.4
 *
 * @brief This test verifies that the IUT detects an error when after the
 *        transmission of 5 identical bits, it receives a sixth bit identical
 *        to the five precedents. This test is executed with extended format
 *        frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ID, RTR, DLC, Data, FDF = 0
 *
 *  CAN FD Enabled
 *      ID, SRR, RRS, BRS, ESI, DLC, Data Byte 0 - other bytes 0x55, FDF = 1
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *   All stuff bits within the defined frames will be tested.
 *
 *   There are 35 elementary tests to perform.
 *
 *              ID              CTRL                DATA
 *      #1      0x07C30F0F      0x188               All byte 0x3C
 *      #2      0x07C0F0F0      0x181               0x00
 *      #3      0x1FB80000      0x181               0xA0
 *      #4      0x00000000      0x181               0x00
 *
 *    For an OPEN device, at least one stuff error shall be generated at each
 *    stuffed field.
 *
 *    For a SPECIFIC device, at least one stuff error shall be generated at each
 *    stuffed field, where a stuff bit can occur.
 *
 *  CAN FD enabled
 *      All stuff bits up to the second payload byte within the defined frames
 *      will be tested.
 *
 *      There are 79 elementary tests to perform.
 *              ID              CTRL                DATA
 *      #1      0x01E38787      0x6AE               0xF8
 *      #2      0x11F38787      0x6A8               0x3C
 *      #3      0x1079C1E1      0x6BE               0x1E
 *      #4      0x083DF0F0      0x69F               0x0F
 *      #5      0x041EF878      0x68F               0x87
 *      #6      0x1F0C3C3C      0x683               0xC3
 *      #7      0x0F861E1E      0x6A3               0xE1
 *      #8      0x07C30F0F      0x6A1               0xF0
 *      #9      0x1C3FC3C3      0x6A0               -
 *     #10      0x020FE1FF      0x6B0               -
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit the frames and creates a stuff error
 *  according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame at the bit position following the
 *  corrupted stuff bit.
 *  The IUT shall restart the transmission of the data frame as soon as the
 *  bus is idle.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_2_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 4; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
            for (size_t i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            SetupMonitorTxTests();
            /* TX to RX feedback must be disabled since we corrupt dominant bits to Recessive */
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int id = 0;
            uint8_t dlc = 0;
            uint8_t data[64] = {};

            /* Variants differ only in value of reserved bit! CAN 2.0 shall accept FDF recessive
             * and CAN FD Tolerant shall go to protocol exception!
             */
            if (test_variant == TestVariant::Common)
            {
                switch (elem_test.index_)
                {
                case 1:
                    id = 0x07C30F0F;
                    dlc = 0x8;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Ext, RtrFlag::Data);
                    for (size_t i = 0; i < 8; i++)
                        data[i] = 0x3C;
                    break;

                case 2:
                    id = 0x07C0F0F0;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Ext, RtrFlag::Data);
                    data[0] = 0x00;
                    break;

                case 3:
                    id = 0x1FB80000;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Ext, RtrFlag::Data);
                    data[0] = 0xA0;
                    break;

                case 4:
                    id = 0x00000000;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Ext, RtrFlag::Data);
                    data[0] = 0x00;
                    break;

                default:
                    break;
                }

            }
            else if (test_variant == TestVariant::CanFdEna)
            {
                switch (elem_test.index_)
                {
                case 1:
                    id = 0x01E38787;
                    dlc = 0xE;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    data[0] = 0xF8;
                    break;

                case 2:
                    id = 0x11F38787;
                    dlc = 0x8;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    data[0] = 0x3C;
                    break;

                case 3:
                    id = 0x1079C1E1;
                    dlc = 0xE;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrPas);
                    data[0] = 0x1E;
                    break;

                case 4:
                    id = 0x083DF0F0;
                    dlc = 0xF;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrPas);
                    data[0] = 0x0F;
                    break;

                case 5:
                    id = 0x041EF878;
                    dlc = 0xF;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrAct);
                    data[0] = 0x87;
                    break;

                case 6:
                    id = 0x1F0C3C3C;
                    dlc = 0x3;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrAct);
                    data[0] = 0xC3;
                    break;

                case 7:
                    id = 0x0F861E1E;
                    dlc = 0x3;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    data[0] = 0xE1;
                    break;

                case 8:
                    id = 0x07C30F0F;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    data[0] = 0xF0;
                    break;

                case 9:
                    id = 0x1C3FC3C3;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    break;

                case 10:
                    id = 0x020FE1FF;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Ext, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrPas);
                    break;

                default:
                    break;
                }

                for (size_t i = 1; i < 64; i++)
                    data[i] = 0x55;
            }

            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, id, data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
            * Modify test frames:
            *   1. Update the frame since number of stuff bits might have changed.
            *   2. Pick one of the stuff bits within the frame and flip its value.
            *   3. Insert Error frame to monitored and driven frame (TX/RX feedback
            *      disabled). Error frame can be passive or active based on Fault state of IUT!
            *   4. Append retransmitted frame to both driven and monitored frames!
            ***************************************************************************************/

            // Elementary tests with ESI=1 means that IUT must be error passive to send such frame!
            bool is_err_passive = false;
            if (test_variant == TestVariant::CanFdEna &&
                ((elem_test.index_ == 3) ||
                    (elem_test.index_ == 4) ||
                    (elem_test.index_ == 10)))
                is_err_passive = true;

            drv_bit_frm->UpdateFrame();
            mon_bit_frm->UpdateFrame();

            size_t num_stuff_bits = drv_bit_frm->GetNumStuffBits(StuffKind::Normal);

            /* In FD enabled variant, if last bit of data field is stuff bit, but model has this bit
             * as fixed stuff bit before Stuff count. So count in also each fixed stuff bit even
             * if last bit of data is NOT regular stuff bit. Then total number of stuff bits within
             * FD enabled variant will be higher than in ISO 16845, but this does not mind!
             */
            if (test_variant == TestVariant::CanFdEna)
            {
                Bit *bit = drv_bit_frm->GetBitOf(0, BitKind::StuffCnt);
                size_t index = drv_bit_frm->GetBitIndex(bit);
                BitVal value = drv_bit_frm->GetBit(index - 1)->val_;
                if ((value == drv_bit_frm->GetBit(index - 2)->val_) &&
                    (value == drv_bit_frm->GetBit(index - 3)->val_) &&
                    (value == drv_bit_frm->GetBit(index - 4)->val_) &&
                    (value == drv_bit_frm->GetBit(index - 5)->val_))
                    num_stuff_bits++;
            }

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            for (size_t stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
            {
                TestMessage("Testing stuff bit nr: %zu", stuff_bit);
                stuff_bits_in_variant++;

                /*
                 * Copy frame to second frame so that we dont loose modification of bits.
                 * Corrupt only second one.
                 */
                drv_bit_frm_2 = std::make_unique<BitFrame>(*drv_bit_frm);
                mon_bit_frm_2 = std::make_unique<BitFrame>(*mon_bit_frm);

                Bit *stuff_bit_to_flip = drv_bit_frm_2->GetStuffBit(stuff_bit);
                size_t bit_index = drv_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                stuff_bit_to_flip->FlipVal();

                if (is_err_passive)
                {
                    drv_bit_frm_2->InsertPasErrFrm(bit_index + 1);
                    mon_bit_frm_2->InsertPasErrFrm(bit_index + 1);
                    drv_bit_frm_2->AppendSuspTrans();
                    mon_bit_frm_2->AppendSuspTrans();
                }
                else
                {
                    drv_bit_frm_2->InsertActErrFrm(bit_index + 1);
                    mon_bit_frm_2->InsertActErrFrm(bit_index + 1);
                }

                /* Append retransmitted frame */
                drv_bit_frm_3 = std::make_unique<BitFrame>(*drv_bit_frm);
                mon_bit_frm_3 = std::make_unique<BitFrame>(*mon_bit_frm);

                drv_bit_frm_3->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

                drv_bit_frm_2->AppendBitFrame(drv_bit_frm_3.get());
                mon_bit_frm_2->AppendBitFrame(mon_bit_frm_3.get());

                drv_bit_frm_2->Print(true);
                mon_bit_frm_2->Print(true);

                /* Do the test itself */
                if (is_err_passive)
                    dut_ifc->SetTec(150);
                else
                    dut_ifc->SetTec(0);

                PushFramesToLT(*drv_bit_frm_2, *mon_bit_frm_2);
                StartDrvAndMon();
                dut_ifc->SendFrame(gold_frm.get());
                WaitForDrvAndMon();
                CheckLTResult();

                drv_bit_frm_2.reset();
                mon_bit_frm_2.reset();
                drv_bit_frm_3.reset();
                mon_bit_frm_3.reset();
            }

            return FinishElemTest();
        }

};