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
 * @date 18.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.6
 *
 * @brief The purpose of this test is to verify that an IUT correctly generates
 *        the stuff bits in a base format frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          ID, RTR, DATA, DLC, FDF = 0
 *      CAN FD enabled:
 *          ID, RRS, BRS, ESI, DLC, DATA, FDF = 1
 *
 * Elementary test cases:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          For an OPEN device, there are six elementary tests to perform:
 *                          CBFF
 *              ID          CTRL        DATA
 *       #1    0x78         0x08      first byte: 0x01, others: 0xE1
 *       #2   0x41F         0x01      0x00
 *       #3   0x47F         0x01      0x1F
 *       #4   0x758         0x00       -
 *       #5   0x777         0x01      0x1F
 *       #6   0x7EF         0x42       -
 *
 *      CAN FD Enabled:
 *          The following cases are tested:
 *                          FBFF
 *             ID           CTRL        DATA
 *       #1    0x78        0x0AE      0xF8, all other bytes 0x78
 *       #2   0x4C7        0x0A8      all bytes 0x3C
 *       #3   0x41E        0x0BE      all bytes 0x1E
 *       #4   0x20F        0x09F      all bytes 0x0F
 *       #5   0x107        0x08F      all bytes 0x87
 *       #6   0x7C3        0x083      all bytes 0xC3
 *       #7   0x3E1        0x0A3      all bytes 0xE1
 *       #8   0x1F0        0x0A1      all bytes 0xF0
 *       #9   0x000        0x0A0      -
 *      #10   0x7FF                   0xB0
 *          There are 10 elementary tests to perform.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall correctly generate all stuff bits.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_1_6 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (int i = 0; i < 6; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
            for (int i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            /* Basic setup for tests where IUT transmits */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            if (test_variant == TestVariant::Common)
            {
                /* Last iteration (0x42 CTRL field) indicates RTR frame */
                RtrFlag rtr_flag;
                if (elem_test.index_ == 6)
                    rtr_flag = RtrFlag::Rtr;
                else
                    rtr_flag = RtrFlag::Data;

                frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                                        IdentKind::Base, rtr_flag);

                /* Data, DLCs and identifiers for each iteration */
                uint8_t data[6][8] = {
                    {0x01, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
                };
                uint8_t dlcs[6] = {
                    0x8, 0x1, 0x1, 0x0, 0x1, 0x2
                };
                int ids[6] = {
                    0x78, 0x41F, 0x47F, 0x758, 0x777, 0x7EF
                };
                gold_frm = std::make_unique<Frame>(*frm_flags, dlcs[elem_test.index_ - 1],
                                    ids[elem_test.index_ - 1], data[elem_test.index_ -1]);

            } else if (test_variant == TestVariant::CanFdEna) {

                /* Flags based on elementary test */
                switch(elem_test.index_)
                {
                    case 1:
                    case 2:
                    case 7:
                    case 8:
                    case 9:
                        frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrAct);
                        break;

                    case 3:
                        frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::DoShift, EsiFlag::ErrPas);
                        break;

                    case 4:
                        frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrPas);
                        break;

                    case 5:
                    case 6:
                        frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrAct);
                        break;

                    case 10:
                        frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                            IdentKind::Base, RtrFlag::Data,
                                            BrsFlag::NoShift, EsiFlag::ErrAct);
                        break;
                    default:
                        break;
                };

                /* DUT must be set to error passive state when ErrorPassive is expected!
                    * Otherwise, it would transmitt ESI_ERROR_ACTIVE
                    */
                if (elem_test.index_ == 3 || elem_test.index_ == 4)
                    dut_ifc->SetErrorState(FaultConfState::ErrPas);
                else
                    dut_ifc->SetErrorState(FaultConfState::ErrAct);

                int ids[10] = {
                    0x78, 0x47C, 0x41E, 0x20F, 0x107, 0x7C3, 0x3E1, 0x1F0, 0x000, 0x7FF
                };

                /* Data based on elementary test */
                uint8_t data[10][64] = {
                    {
                        0xF8, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F
                    },
                    {
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87
                    },
                    {
                        0xC3, 0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    { // Dont care since DLC = 0
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0
                    }
                };

                uint8_t dlcs[10] = {
                    0xE, 0x8, 0xE, 0xF, 0xF, 0x3, 0x3, 0x1, 0x0, (uint8_t)(rand() % 0xF)
                };
                gold_frm = std::make_unique<Frame>(*frm_flags, dlcs[elem_test.index_ - 1],
                                    ids[elem_test.index_ - 1], data[elem_test.index_ - 1]);
            }

            /* Randomize will have no effect since everything is specified */
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received (insert ACK).
             *
             * No other modifications are needed as correct stuff generation is verified by model!
             **************************************************************************************/
            drv_bit_frm->ConvRXFrame();

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            this->dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }

};