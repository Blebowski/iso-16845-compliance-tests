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
 * @date 19.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.10
 *
 * @brief This test verifies the behaviour of the IUT when receiving a correct
 *        base format frame with particular data containing critical stuffing
 *        bit profiles in the different fields of the frame according to test
 *        variables.
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
 *          ID, RRS, BRS, ESI, DLC, DATA
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
 *      #3    0x707         0x0F              all bytes 0x0F
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
 *      #1     0x78         0xAE              0xF8, all others 0x78
 *      #2    0x47C         0xA8              all bytes 0x3C
 *      #3    0x41E         0xBE              all bytes 0x1E
 *      #4    0x20F         0x9F              all bytes 0x0F
 *      #5    0x107        0x28F              all bytes 0x87
 *      #6    0x7C3         0x83              all bytes 0xC3
 *      #7    0x3E1         0xA3              all bytes 0xE1
 *      #8    0x1F0         0xA1              0xF0
 *      #9    0x000         0xA0                -
 *     #10    0x7FF         0xB0                -
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for each of the elementary tests.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_10 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::ClasCanFdCommon);
            for (auto const &test_variant : test_variants)
                for (size_t j = 0; j < 10; j++)
                    AddElemTest(test_variant, ElemTest(j + 1));

            CanAgentConfigureTxToRxFeedback(true);
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
            if (test_variant == TestVariant::Can20 || test_variant == TestVariant::CanFdTol)
            {
                switch (elem_test.index_)
                {
                case 1:
                    id = 0x78;
                    dlc = 0x8;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    data[0] = 0x01;
                    for (int i = 1; i < 8; i++)
                        data[i] = 0xE1;
                    break;

                case 2:
                    id = 0x41F;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    data[0] = 0x00;
                    break;

                case 3:
                    id = 0x707;
                    dlc = 0xF;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    for (int i = 0; i < 8; i++)
                        data[i] = 0x0F;
                    break;

                case 4:
                    id = 0x360;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    break;

                case 5:
                    id = 0x730;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    break;

                case 6:
                    id = 0x47F;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    data[0] = 0x1F;
                    break;

                case 7:
                    id = 0x758;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    break;

                case 8:
                    id = 0x777;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Data);
                    data[0] = 0x1F;
                    break;

                case 9:
                    id = 0x7EF;
                    dlc = 0x2;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Rtr);
                    break;

                case 10:
                    id = 0x3EA;
                    dlc = 0xF;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                            IdentKind::Base, RtrFlag::Rtr);
                    break;

                default:
                    TestMessage("Invalid Elementary test index: %zu", elem_test.index_);
                    break;
                }

            }
            else if (test_variant == TestVariant::CanFdEna)
            {
                switch (elem_test.index_)
                {
                case 1:
                    id = 0x78;
                    dlc = 0xE;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    data[0] = 0x01;
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xE1;
                    break;

                case 2:
                    id = 0x47C;
                    dlc = 0x8;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x3C;
                    break;

                case 3:
                    id = 0x41E;
                    dlc = 0xE;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrPas);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x1E;
                    break;

                case 4:
                    id = 0x20F;
                    dlc = 0xF;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrPas);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x0F;
                    break;

                case 5:
                    id = 0x107;
                    dlc = 0xF;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrAct);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x87;
                    break;

                case 6:
                    id = 0x7C3;
                    dlc = 0x3;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrAct);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xC3;
                    break;

                case 7:
                    id = 0x3E1;
                    dlc = 0x3;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xE1;
                    break;

                case 8:
                    id = 0x1F0;
                    dlc = 0x1;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xF0;
                    break;

                case 9:
                    id = 0x000;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                    break;

                case 10:
                    id = 0x7FF;
                    dlc = 0x0;
                    frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrPas);
                    break;

                default:
                    break;
                }
            }

            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, id, data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Modify some of the bits as per elementary test cases
             *   2. Update the frame since number of stuff bits might have changed.
             *   3. Turn monitored frame to received.
             *************************************************************************************/
            if (test_variant == TestVariant::Can20)
            {
                switch(elem_test.index_)
                {
                case 3:
                case 4:
                case 5:
                case 10:
                    drv_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
                    mon_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
                default:
                    break;
                }
            }
            else if (test_variant == TestVariant::CanFdEna)
            {
                if (elem_test.index_ == 5)
                {
                    drv_bit_frm->GetBitOf(0, BitKind::R1)->val_ = BitVal::Recessive;
                    mon_bit_frm->GetBitOf(0, BitKind::R1)->val_ = BitVal::Recessive;
                }
            }

            drv_bit_frm->UpdateFrame();
            mon_bit_frm->UpdateFrame();

            mon_bit_frm->ConvRXFrame();

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            FreeTestObjects();
            return FinishElemTest();
        }
};
