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
 * @date 13.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.2.2
 *
 * @brief This test verifies that the IUT detects a stuff error whenever it
 *        receives 6 consecutive bits of the same value until the position of
 *        the CRC delimiter in a base format frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *      Classical CAN
 *          ID, RTR, FDF, DLC, DATA
 *
 *      CAN FD Tolerant, CAN FD Enabled
 *          ID, RTR, DLC, DATA
 *
 *      CAN FD Enabled
 *          ID, RRS, BRS, ESI, DLC, DATA Byte 0 defined, all others 0x55
 *
 * Elementary test cases:
 *                               Classical CAN
 *              ID          CTRL                DATA
 *      #1     0x78         0x08              0x01, all others 0xE1
 *      #2    0x41F         0x01              0x00
 *      #3    0x707         0x1F              all bytes 0x0F
 *      #4    0x360         0x10                -
 *      #5    0x730         0x10                -
 *      #6    0x47F         0x01              0x1F
 *      #7    0x758         0x00                -
 *      #8    0x777         0x01              0x1F
 *      #9    0x7EF         0x42                -
 *     #10    0x3EA         0x5F                -
 *
 *                      CAN FD Tolerant, CAN FD Enabled
 *              ID          CTRL                DATA
 *      #1     0x78         0x08              0x01, all others 0xE1
 *      #2    0x41F         0x01              0x00
 *      #3    0x707         0x0F              all bytes 0x87
 *      #4    0x360         0x00                -
 *      #5    0x730         0x00                -
 *      #6    0x47F         0x01              0x1F
 *      #7    0x758         0x00                -
 *      #8    0x777         0x01              0x1F
 *      #9    0x7EF         0x42                -
 *     #10    0x3EA         0x4F                -
 *
 *                              CAN FD Enabled
 *              ID          CTRL                DATA
 *      #1     0x78         0xAE              0xF8, all others 0x55
 *      #2    0x47C         0xA8              0x3C, all others 0x55
 *      #3    0x41E         0xBE              0x1E, all others 0x55
 *      #4    0x20F         0x9F              0x0F, all others 0x55
 *      #5    0x107        0x28F              0x87, all others 0x55
 *      #6    0x7C3         0x83              0xC3, all others 0x55
 *      #7    0x3E1         0xA3              0xE1, all others 0x55
 *      #8    0x1F0         0xA1              0xF0, all others 0x55
 *      #9    0x000         0xA0                -
 *     #10    0x7FF         0xB0                -
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for each elementary test. The LT forces one
 *  of the stuff bits to its complement.
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

class TestIso_7_2_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::ClassicalFdCommon);
            for (const auto &test_variant : test_variants)
                for (size_t j = 0; j < 10; j++)
                    AddElemTest(test_variant, ElementaryTest(j + 1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int id = 0;
            uint8_t dlc = 0;
            uint8_t data[64] = {};

            /* Variants differ only in value of reserved bit! CAN 2.0 shall accept FDF recessive
             * and CAN FD Tolerant shall go to protocol exception!
             */
            if (test_variant == TestVariant::Can_2_0 || test_variant == TestVariant::CanFdTolerant)
            {
                switch (elem_test.index_)
                {
                case 1:
                    id = 0x78;
                    dlc = 0x8;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x01;
                    for (int i = 1; i < 8; i++)
                        data[i] = 0xE1;
                    break;

                case 2:
                    id = 0x41F;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x00;
                    break;

                case 3:
                    id = 0x707;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    for (int i = 0; i < 8; i++)
                        if (test_variant == TestVariant::Can_2_0)
                            data[i] = 0x0F;
                        else
                            data[i] = 0x87;
                    break;

                case 4:
                    id = 0x360;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 5:
                    id = 0x730;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 6:
                    id = 0x47F;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x1F;
                    break;

                case 7:
                    id = 0x758;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 8:
                    id = 0x777;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x1F;
                    break;

                case 9:
                    id = 0x7EF;
                    dlc = 0x2;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::RtrFrame);
                    break;

                case 10:
                    id = 0x3EA;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::RtrFrame);
                default:
                    break;
                }

            }
            else if (test_variant == TestVariant::CanFdEnabled)
            {
                switch (elem_test.index_)
                {
                case 1:
                    id = 0x78;
                    dlc = 0xE;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    data[0] = 0xF8;
                    break;

                case 2:
                    id = 0x47C;
                    dlc = 0x8;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    data[0] = 0x3C;
                    break;

                case 3:
                    id = 0x41E;
                    dlc = 0xE;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorPassive);
                    data[0] = 0x1E;
                    break;

                case 4:
                    id = 0x20F;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorPassive);
                    data[0] = 0x0F;
                    break;

                case 5:
                    id = 0x107;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorActive);
                    data[0] = 0x87;
                    break;

                case 6:
                    id = 0x7C3;
                    dlc = 0x3;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorActive);
                    data[0] = 0xC3;
                    break;

                case 7:
                    id = 0x3E1;
                    dlc = 0x3;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    data[0] = 0xE1;
                    break;

                case 8:
                    id = 0x1F0;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    data[0] = 0xF0;
                    break;

                case 9:
                    id = 0x000;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    break;

                case 10:
                    id = 0x7FF;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorPassive);
                    break;

                default:
                    break;
                }

                for (int i = 1; i < 64; i++)
                    data[i] = 0x55;
            }

            golden_frm = std::make_unique<Frame>(*frame_flags, dlc, id, data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Modify some of the bits as per elementary test cases
             *   2. Update the frame since number of stuff bits might have changed.
             *   3. Turn monitored frame to received.
             *   4. Pick one of the stuff bits within the frame and flip its value.
             *   5. Insert Active Error frame to monitored frame. Insert Passive Error frame to
             *      driven frame (TX/RX feedback enabled).
             *************************************************************************************/
            if (test_variant == TestVariant::Can_2_0)
            {
                switch(elem_test.index_)
                {
                case 3:
                case 4:
                case 5:
                case 10:
                    driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                default:
                    break;
                }
            }
            else if (test_variant == TestVariant::CanFdEnabled)
            {
                if (elem_test.index_ == 5)
                {
                    driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                }
            }

            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            monitor_bit_frm->TurnReceivedFrame();

            int num_stuff_bits = driver_bit_frm->GetNumStuffBits(StuffBitType::NormalStuffBit);

            /* In FD enabled variant, if last bit of data field is stuff bit, but model has this bit
             * as fixed stuff bit before Stuff count. So count in also each fixed stuff bit even
             * if last bit of data is NOT regular stuff bit. Then total number of stuff bits within
             * FD enabled variant will be higher than in ISO 16845, but this does not mind!
             */
            if (test_variant == TestVariant::CanFdEnabled)
            {
                Bit *bit = driver_bit_frm->GetBitOf(0, BitType::StuffCount);
                int index = driver_bit_frm->GetBitIndex(bit);
                BitValue value = driver_bit_frm->GetBit(index - 1)->bit_value_;
                if ((value == driver_bit_frm->GetBit(index - 2)->bit_value_) &&
                    (value == driver_bit_frm->GetBit(index - 3)->bit_value_) &&
                    (value == driver_bit_frm->GetBit(index - 4)->bit_value_) &&
                    (value == driver_bit_frm->GetBit(index - 5)->bit_value_))
                    num_stuff_bits++;
            }

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            for (int stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
            {
                TestMessage("Testing stuff bit nr: %d", stuff_bit);
                TestMessage("Total stuff bits in variant so far: %d", stuff_bits_in_variant);
                stuff_bits_in_variant++;

                /*
                 * Copy frame to second frame so that we dont loose modification of bits.
                 * Corrupt only second one.
                 */
                driver_bit_frm_2 = std::make_unique<BitFrame>(*driver_bit_frm);
                monitor_bit_frm_2 = std::make_unique<BitFrame>(*monitor_bit_frm);

                Bit *stuff_bit_to_flip = driver_bit_frm_2->GetStuffBit(stuff_bit);
                int bit_index = driver_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                /* Here we only flip, no compensation! This is because since we
                 * flip stuff bit, we remove the synchronization edge, therefore there
                 * is no need to compensate the edge position!
                 */
                stuff_bit_to_flip->FlipBitValue();

                driver_bit_frm_2->InsertPassiveErrorFrame(bit_index + 1);
                monitor_bit_frm_2->InsertActiveErrorFrame(bit_index + 1);

                /* Do the test itself */
                dut_ifc->SetRec(0);
                PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                driver_bit_frm_2.reset();
                monitor_bit_frm_2.reset();
            }

            FreeTestObjects();
            return FinishElementaryTest();
        }
};
