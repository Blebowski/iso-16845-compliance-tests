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

#include "pli_lib.h"

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

class TestIso_8_6_6 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 8; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
            for (int i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            do
            {
                TestBigMessage("Generating random frame...");
                frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, RtrFlag::DataFrame,
                                                           EsiFlag::ErrorActive);
                uint8_t dlc;

                if (test_variant == TestVariant::CanFdEnabled)
                {
                    /* To achieve CRC17 or CRC21 in elem tests 7-10 of Can FD Enabled variant */
                    if (elem_test.index_ == 7 || elem_test.index_ == 8)
                        dlc = (rand() % 0xA) + 1;
                    else if (elem_test.index_ == 9 || elem_test.index_ == 10)
                        dlc = (rand() % 5) + 0xB;
                    else
                        dlc = rand() % 0xF;
                    } else {
                        dlc = rand() % 8 + 1;
                    }
                    golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
                    RandomizeAndPrint(golden_frm.get());

            // We repeat generating random frame as long as we have any of the fields
            // whose non-stuff bits we could possibly flip with all zeroes or all ones!
            } while (golden_frm->identifier() == (pow(2, 11) - 1) ||
                     golden_frm->identifier() == (pow(2, 29) - 1) ||
                     golden_frm->dlc() == 0x0 ||
                     golden_frm->dlc() == 0xF ||
                     golden_frm->data(0) == 0x00 ||
                     golden_frm->data(0) == 0xFF);

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Corrupt bit as given by elementary test case. Avoid corrupting stuff bits
             *      alltogether!
             *   2. Insert active error frame to both driven and monitored frames from next bit on.
             *   3. Append the same frame again with ACK on driven frame. This emulates retransmi-
             *      tted frame by IUT.
             *************************************************************************************/
            BitType bit_type_to_corrupt = BitType::Sof;
            BitValue value_to_corrupt = BitValue::Dominant;
            switch (elem_test.index_)
            {
            case 1:
                bit_type_to_corrupt = BitType::Sof;
                break;
            case 2:
                bit_type_to_corrupt = BitType::BaseIdentifier;
                break;
            case 3:
                bit_type_to_corrupt = BitType::Dlc;
                break;
            case 4:
                bit_type_to_corrupt = BitType::Dlc;
                value_to_corrupt = BitValue::Recessive;
                break;
            case 5:
                bit_type_to_corrupt = BitType::Data;
                break;
            case 6:
                bit_type_to_corrupt = BitType::Data;
                value_to_corrupt = BitValue::Recessive;
                break;
            case 7:
            case 9:
                bit_type_to_corrupt = BitType::Crc;
                break;
            case 8:
            case 10:
                bit_type_to_corrupt = BitType::Crc;
                value_to_corrupt = BitValue::Recessive;
                break;
            default:
                break;
            }

            /* Find random bit within bitfield with value */
            Bit *bit_to_corrupt = driver_bit_frm->GetRandomBitOf(bit_type_to_corrupt);
            while (bit_to_corrupt->bit_value_ != value_to_corrupt ||
                   bit_to_corrupt->stuff_bit_type_ != StuffBitType::NoStuffBit)
                bit_to_corrupt = driver_bit_frm->GetRandomBitOf(bit_type_to_corrupt);
            // TODO: CRC can be possibly all zeroes or all ones causing infinite loop!

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
            /* +8 for error, -1 for succsefull retransmission */
            CheckTecChange(tec_old, 7);

            return FinishElementaryTest();
        }

};