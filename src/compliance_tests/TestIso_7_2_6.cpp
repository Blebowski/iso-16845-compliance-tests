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
 * @date 11.4.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.2.6
 *
 * @brief The purpose of this test is to verify that an IUT detecting a CRC
 *        error and a form error on the CRC delimiter in the same frame
 *        generates only one single 6 bits long error flag starting on the bit
 *        following the CRC delimiter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter, FDF = 0
 *
 *  CAN FD Enabled
 *      CRC, DLC - to cause different CRC types. FDF = 1
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      #1 CRC (15)
 *
 *  CAN FD Enabled
 *      #1 DLC ≤ 10 − > CRC (17)
 *      #2 DLC > 10 − > CRC (21)
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with CRC error and form error at CRC delimiter
 *  according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate one active error frame starting at the bit position
 *  following the CRC delimiter.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

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

class TestIso_7_2_6: public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(2, FrameType::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc;
            if (test_variant == TestVariant::Common)
            {
                dlc = rand() % 9;
            }
            else if (elem_test.index_ == 1)
            {
                if (rand() % 2)
                    dlc = 0x9;
                else
                    dlc = 0xA;
            } else
            {
                dlc = (rand() % 5) + 11;
            }

            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force random CRC bit to its opposite value
             *   3. Force CRC Delimiter to dominant.
             *   4. Insert Error frame to position of ACK!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            do {
                Bit *bit = driver_bit_frm->GetRandomBitOf(BitType::Crc);
                int index = driver_bit_frm->GetBitIndex(bit);

                // Ignore stuff bits, and bits just before stuff bits. If we flip bit before
                // stuff bit, then we cause stuff error too!
                if (bit->stuff_bit_type_ == StuffBitType::NoStuffBit &&
                    driver_bit_frm->GetBit(index + 1)->stuff_bit_type_ == StuffBitType::NoStuffBit)
                    {
                        // This should cause only CRC error, no stuff error!
                        bit->FlipBitValue();
                        break;
                    }
            } while (true);

            // If we are in CAN 2.0, then flipping also non-stuff bit can cause change of
            // CRC lenght since number of stuff bits can change! Therefore, we need to recalculate
            // stuff bits, but keep the CRC (since it contains corrupted bit that we rely on)!
            driver_bit_frm->UpdateFrame(false);

            driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Ack);
            driver_bit_frm->InsertActiveErrorFrame(0, BitType::Ack);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
};