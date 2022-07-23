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
 * @test ISO16845 8.2.3
 * 
 * @brief This test verifies that the IUT detects an error when after the
 *        transmission of 5 identical bits, it receives a sixth bit identical
 *        to the five precedents. This test is executed with a base format
 *        frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ID, RTR, DLC, Data, FDF = 0
 * 
 *  CAN FD Enabled
 *      ID, RTR, DLC, DATA byte 0 defined in test case, all other 
 *      DATA byte = 55 h, FDF = 1
 * 
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *   All stuff bits within the defined frames will be tested.
 *
 *   There are 35 elementary tests to perform.
 * 
 *              ID              CTRL                DATA
 *      #1      0x78            0x08                0x01, all other bytes 0xE1
 *      #2      0x41F           0x01                0x00
 *      #3      0x47F           0x01                0x1F
 *      #4      0x758           0x00                -
 *      #5      0x777           0x01                0x1F
 *      #6      0x7EF           0x42                -
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
 *      There are 39 elementary tests to perform.
 *             ID              CTRL                DATA
 *      #1     0x78            0x0AE               0xF8   
 *      #2     0x47C           0x0A8               0x3C
 *      #3     0x41E           0x0BE               0x1E
 *      #4     0x20F           0x09F               0x0F
 *      #5     0x107           0x08F               0x87
 *      #6     0x7C3           0x083               0xC3
 *      #7     0x3E1           0x0A3               0xE1
 *      #8     0x1F0           0x0A1               0xF0
 *      #9     0x000           0x0A0               -
 *     #10     0x7FF           0x0B0               -
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

#include "../vpi_lib/vpiComplianceLib.hpp"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"
#include "../test_lib/TestLoader.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;
using namespace test_lib;

class TestIso_8_2_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 6; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
            for (int i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            SetupMonitorTxTests();
            /* TX to RX feedback must be disabled since we corrupt dominant bits to Recessive */
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int id = 0;
            uint8_t dlc = 0;
            uint8_t data[64] = {};

            /* Variants differ only in value of reserved bit! CAN 2.0 shall accept FDF
             * recessive and CAN FD Tolerant shall go to protocol exception!
             */
            if (test_variant == TestVariant::Common)
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
                    id = 0x47F;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x1F;
                    break;

                case 4:
                    id = 0x758;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 5:
                    id = 0x777;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x1F;
                    break;

                case 6:
                    id = 0x7EF;
                    dlc = 0x2;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::RtrFrame);
                    data[0] = 0x1F;
                    break;

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
             *   1. Update the frame since number of stuff bits might have changed.
             *   2. Pick one of the stuff bits within the frame and flip its value.
             *   3. Insert Error frame to monitored and driven frame (TX/RX feedback disabled).
             *      Error frame can be passive or active based on Fault state of IUT!
             *   4. Append retransmitted frame to both driven and monitored frames!
             *************************************************************************************/

            // Elementary tests with ESI=1 means that IUT must be error passive to send such frame!
            bool is_err_passive = false;
            if (test_variant == TestVariant::CanFdEnabled &&
                ((elem_test.index_ == 3) ||
                 (elem_test.index_ == 4) ||
                 (elem_test.index_ == 10)))
                is_err_passive = true;

            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            int num_stuff_bits = driver_bit_frm->GetNumStuffBits(StuffBitType::NormalStuffBit);

            /*****************************************************************************
             * Execute test
             ****************************************************************************/
            for (int stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
            {
                TestMessage("Testing stuff bit nr: %d", stuff_bit);
                stuff_bits_in_variant++;

                /* 
                 * Copy frame to second frame so that we dont loose modification of bits.
                 * Corrupt only second one.
                 */
                driver_bit_frm_2 = std::make_unique<BitFrame>(*driver_bit_frm);
                monitor_bit_frm_2 = std::make_unique<BitFrame>(*monitor_bit_frm);

                Bit *stuff_bit_to_flip = driver_bit_frm_2->GetStuffBit(stuff_bit);
                int bit_index = driver_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                stuff_bit_to_flip->FlipBitValue();

                if (is_err_passive)
                {
                    driver_bit_frm_2->InsertPassiveErrorFrame(bit_index + 1);
                    monitor_bit_frm_2->InsertPassiveErrorFrame(bit_index + 1);
                    driver_bit_frm_2->AppendSuspendTransmission();
                    monitor_bit_frm_2->AppendSuspendTransmission();
                }
                else
                {
                    driver_bit_frm_2->InsertActiveErrorFrame(bit_index + 1);
                    monitor_bit_frm_2->InsertActiveErrorFrame(bit_index + 1);
                }

                /* Append retransmitted frame */
                driver_bit_frm_3 = std::make_unique<BitFrame>(*driver_bit_frm);
                monitor_bit_frm_3 = std::make_unique<BitFrame>(*monitor_bit_frm);

                driver_bit_frm_3->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                driver_bit_frm_2->AppendBitFrame(driver_bit_frm_3.get());
                monitor_bit_frm_2->AppendBitFrame(monitor_bit_frm_3.get());

                driver_bit_frm_2->Print(true);
                monitor_bit_frm_2->Print(true);

                // Do test itself
                if (is_err_passive)
                    dut_ifc->SetTec(150);
                else
                    dut_ifc->SetTec(0);

                PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
                StartDriverAndMonitor();
                dut_ifc->SendFrame(golden_frm.get());
                WaitForDriverAndMonitor();
                CheckLowerTesterResult();

                driver_bit_frm_2.reset();
                monitor_bit_frm_2.reset();
                driver_bit_frm_3.reset();
                monitor_bit_frm_3.reset();
            }

            return FinishElementaryTest();
        }

};