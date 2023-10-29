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
 * @test ISO16845 8.1.7
 *
 * @brief The purpose of this test is to verify that an IUT correctly generates
 *        the stuff bits in an extended frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          ID, SRR, RTR, DATA, DLC, FDF = 0
 *      CAN FD enabled:
 *          ID, SRR, RRS, BRS, ESI, DLC, DATA, FDF = 1
 *
 * Elementary test cases:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          For an OPEN device, there are three elementary tests to perform:
 *                            CBFF
 *                ID          CTRL        DATA
 *       #1   0x07C30F0F      0x188     all bytes 0x3C
 *       #2   0x07C0F0F0      0x181     0x00
 *       #3   0x1FB80000      0x181     0xA0
 *
 *      CAN FD Enabled:
 *          The following cases are tested:
 *                            FBFF
 *             ID             CTRL        DATA
 *       #1   0x01E38787      0x6AE     0xF8, other bytes 0x78
 *       #2   0x11F3C3C3      0x6A8     all bytes 0x3C
 *       #3   0x1079C1E1      0x6BE     all bytes 0x1E
 *       #4   0x083DF0F0      0x69F     all bytes 0x0F
 *       #5   0x041EF878      0x68F     all bytes 0x87
 *       #6   0x1F0C3C3C      0x683     all bytes 0xC3
 *       #7   0x0F861E1E      0x6A3     all bytes 0xE1
 *       #8   0x07C30F0F      0x6A1     all bytes 0xF0
 *       #9   0x1FFC0000      0x6A0     -
 *      #10   0x0003FFFF      0x6B0     -
 *
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

class TestIso_8_1_7 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 3; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameKind::Can20));
            for (int i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameKind::CanFd));

            /* Basic setup for tests where IUT transmits */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            if (test_variant == TestVariant::Common)
            {
                 frame_flags = std::make_unique<FrameFlags>(FrameKind::Can20,
                                        IdentKind::Ext, RtrFlag::Data);

                // Data, dlcs and identifiers for each iteration
                uint8_t data[3][8] = {
                    {0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
                };
                uint8_t dlcs[] = {
                    0x8, 0x1, 0x1
                };
                int ids[] = {
                    0x07C30F0F, 0x07C30F0F, 0x1FB80000
                };

                golden_frm = std::make_unique<Frame>(*frame_flags, dlcs[elem_test.index_ - 1],
                                ids[elem_test.index_ - 1], data[elem_test.index_ - 1]);

            // CAN FD enabled variant
            } else {

                // Flags based on elementary test
                switch(elem_test.index_)
                {
                    case 1:
                    case 2:
                    case 7:
                    case 8:
                    case 9:
                        frame_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                        IdentKind::Ext, RtrFlag::Data,
                                        BrsFlag::DoShift, EsiFlag::ErrAct);
                        break;

                    case 3:
                    case 10:
                        frame_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                        IdentKind::Ext, RtrFlag::Data,
                                        BrsFlag::DoShift, EsiFlag::ErrPas);
                        break;

                    case 4:
                        frame_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                        IdentKind::Ext, RtrFlag::Data,
                                        BrsFlag::NoShift, EsiFlag::ErrPas);
                        break;

                    case 5:
                    case 6:
                        frame_flags = std::make_unique<FrameFlags>(FrameKind::CanFd,
                                        IdentKind::Ext, RtrFlag::Data,
                                        BrsFlag::NoShift, EsiFlag::ErrAct);
                        break;

                    default:
                        assert(false && " We should never get here!");
                        break;
                }

                // DUT must be set to error passive state when ErrorPassive
                // is expected! Otherwise, it would transmitt ESI_ERROR_ACTIVE
                if (elem_test.index_ == 3 || elem_test.index_ == 4 || elem_test.index_ == 10)
                    dut_ifc->SetErrorState(FaultConfState::ErrPas);
                else
                    dut_ifc->SetErrorState(FaultConfState::ErrAct);

                int ids[] = {
                    0x01E38787, 0x11F3C3C3, 0x1079C1E1, 0x083DF0F0, 0x041EF878,
                    0x1F0C3C3C, 0x0F861E1E, 0x07C30F0F, 0x1FFC0000, 0x0003FFFF
                };

                // Data based on elementary test
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
                    { // Dont care since DLC = 0
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    }
                };

                uint8_t dlcs[] = {
                    0xE, 0x8, 0xE, 0xF, 0xF, 0x3, 0x3, 0x1, 0x0, 0x0
                };
                golden_frm = std::make_unique<Frame>(*frame_flags, dlcs[elem_test.index_ - 1],
                                    ids[elem_test.index_ - 1], data[elem_test.index_ - 1]);
            }

            /* Randomize will have no effect since everything is specified */
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received (insert ACK).
             *
             * No other modifications are needed as correct stuff generation is verified by model!
             *************************************************************************************/
            driver_bit_frm->ConvRXFrame();

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            this->dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }

};