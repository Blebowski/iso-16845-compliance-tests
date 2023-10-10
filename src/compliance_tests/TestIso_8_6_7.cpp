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
 * @date 14.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.6.7
 *
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting a form error in a frame on one of the7
 *        following fields described in test variables.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter
 *      ACK Delimiter
 *      EOF
 *      FDF = 0
 *
 *  CAN FD Enabled
 *      CRC
 *      CRC Delimiter
 *      ACK Delimiter
 *      EOF
 *      DLC - to cause different CRC types
 *      FDF = 1
 *
 * Elementary test cases:
 *   Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *     There are five elementary tests to perform.
 *          #1 CRC Delimiter
 *          #2 ACK Delimiter
 *          #3 EOF first bit
 *          #4 EOF fourth bit
 *          #5 EOF last bit
 *
 *   CAN FD Enabled
 *      There are seven elementary tests to perform.
 *          #1 CRC Delimiter
 *          #2 ACK Delimiter (2 dominant bit in ACK slot)
 *          #3 EOF first bit
 *          #4 EOF fourth bit
 *          #5 EOF last bit
 *          #6 Fix stuff bit in CRC 17
 *          #7 Fix stuff bit in CRC 21
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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_6_7 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 5; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
            for (int i = 0; i < 7; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc;
            if (elem_test.index_ < 7)
                dlc = rand() % 0x9;
            else
                dlc = (rand() % 0x4) + 11;

            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_,
                            //IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                            EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlc);

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Corrupt bit as given by elementary test case.
             *   2. Insert active error frame to both driven and monitored frames from next bit on.
             *   3. Append the same frame again with ACK on driven frame. This emulates
             *      retransmitted frame by IUT.
             *************************************************************************************/
            Bit *bit_to_corrupt;
            switch (elem_test.index_)
            {
            case 1:
                bit_to_corrupt = driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter);
                break;
            case 2:
                bit_to_corrupt = driver_bit_frm->GetBitOf(0, BitType::AckDelimiter);
                break;
            case 3:
                bit_to_corrupt = driver_bit_frm->GetBitOf(0, BitType::Eof);
                break;
            case 4:
                bit_to_corrupt = driver_bit_frm->GetBitOf(3, BitType::Eof);
                break;
            case 5:
                bit_to_corrupt = driver_bit_frm->GetBitOf(6, BitType::Eof);
                break;
            case 6:
            case 7:
                do {
                    bit_to_corrupt = driver_bit_frm->GetRandomBitOf(BitType::Crc);
                } while (bit_to_corrupt->stuff_bit_type_ != StuffBitType::FixedStuffBit);
                break;
            default:
                bit_to_corrupt = driver_bit_frm->GetRandomBitOf(BitType::Crc);
                TestMessage("Invalid Elementary test index: %d", elem_test.index_);
                break;
            }

            /* TX/RX feedback is disabled. We must insert ACK also to driven frame! */
            driver_bit_frm->PutAcknowledge(dut_input_delay);

            driver_bit_frm->FlipBitAndCompensate(bit_to_corrupt, dut_input_delay);
            int bit_index = driver_bit_frm->GetBitIndex(bit_to_corrupt);

            driver_bit_frm->InsertActiveErrorFrame(bit_index + 1);
            monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

            driver_bit_frm_2->PutAcknowledge(dut_input_delay);

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            if (dut_ifc->GetTec() > 100)
                dut_ifc->SetTec(0);

            tec_old = dut_ifc->GetTec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            /* +8 for form error, -1 for sucesfull retransmission */
            CheckTecChange(tec_old, 7);

            return FinishElementaryTest();
        }

};