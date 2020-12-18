/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.12.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.5
 * 
 * @brief The purpose of this test is to verify:
 *          — that the IUT uses the specific CRC mechanism according to frame
 *            format,
 *          — that the IUT detecting a CRC error and generates an error frame
 *            at the correct position, and
 *          — that the IUT does not detect an error when monitoring a dominant
 *            bit at the ACK slot while sending a recessive one.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables: 
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *          CRC, FDF = 0, SOF
 * 
 * Elementary test cases:
 *    Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      Number of elementary tests: 3
 *      #1 A dominant bit in the CRC field is changed in a recessive bit.
 *      #2 A recessive bit in the CRC field is changed in a dominant bit.
 *      #3 The dominant SOF bit in the frame is changed in a recessive
 *         one followed by an ID 001 h.
 * 
 *    CAN FD Enabled:
 *      #1 and #2 A dominant bit in the CRC field is changed in a recessive
 *         bit for CRC-17 with DLC ≤ 10 (#1) and CRC-21 with DLC > 10 (#2)
 *         (test for CRC value).
 *  
 *      #3 and #4 A recessive bit in the CRC field is changed in a dominant
 *         bit for CRC-17 with DLC ≤ 10 (#3) and CRC-21 with DLC > 10 (#4)
 *         (test for CRC value).
 * 
 *      #5 The test system sends a frame where two times a recessive stuff
 *         bit becomes a normal bit by losing one of the previous bits by
 *         synchronization issues while the CRC register is equal zero
 *         (test for stuff-counter).
 * 
 *      #6 The parity bit of the stuff count and the following fixed stuff
 *         bit changed into their opposite values (test for stuff-counter
 *         parity bit value).
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for each elementary test. The LT modifies the
 *  frame according to elementary test cases.
 * 
 * Response:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      The IUT shall not acknowledge the test frame. The IUT shall generate
 *      an active error frame starting at the first bit position following
 *      the ACK delimiter.
 *  
 *  CAN FD Enabled:
 *      The IUT shall not acknowledge the test frame. The IUT shall generate
 *      an active error frame starting at the fourth bit position following the
 *      CRC delimiter.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_7_2_5 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (size_t j = 0; j < 3; j++)
                AddElemTest(TestVariant::Common, ElementaryTest(j + 1, FrameType::Can2_0));
            for (size_t j = 0; j < 6; j++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(j + 1, FrameType::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        /*
         * Choose CRC bit with required value for CRC error insertion. Stuff bits are left out.
         */
        int ChooseCrcBitToCorrupt(BitFrame *bit_frame, BitValue bit_value)
        {
            Bit *bit;
            bool is_ok;
            do {
                is_ok = true;
                bit = bit_frame->GetRandomBitOf(BitType::Crc);
                if (bit->stuff_bit_type != StuffBitType::NoStuffBit)
                    is_ok = false;
                if (bit->bit_value_ != bit_value)
                    is_ok = false;
            } while (!is_ok);
            return bit_frame->GetBitIndex(bit);
        }

        /*
         * Inserts CRC error on bit index position. Flips the bit and updates the frame,
         * so that CRC lenght will be correct as when IUT will receive such sequence!
         */
        void InsertCrcError(int index)
        {
            Bit *drv_bit = driver_bit_frm->GetBit(index);
            Bit *mon_bit = monitor_bit_frm->GetBit(index);

            drv_bit->FlipBitValue();
            mon_bit->FlipBitValue();

            driver_bit_frm->UpdateFrame(false);
            monitor_bit_frm->UpdateFrame(false);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            int id = rand() % (int)pow(2, 11);
            uint8_t dlc;

            if (test_variant == TestVariant::Common) {
                if (elem_test.index == 3) {
                    id = 0x1;
                }
            } else if (test_variant == TestVariant::CanFdEnabled) {
                if (elem_test.index == 1 || elem_test.index == 3) {
                    dlc = rand() % 11; // To cause CRC 17
                } else if (elem_test.index == 2 || elem_test.index == 4) {
                    dlc = rand() % 5 + 11; // To cause CRC 21
                } else {
                    dlc = rand() % 15;
                }
            }

            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlc, id);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Modify bit as given by elementary test. Re-stuff CRC since lenght of
             *      CRC might have changed due to stuff error. This is needed since, flipped CRC
             *      bit might have caused change of CRC length due to stuff bits!
             *   2. Turn monitored frame as received. Force ACK low, since IUT shall not
             *      acknowledge the frame.
             *   3. Insert Active error frame to monitored frame after ACK delimiter. This covers
             *      also CAN FD enabled option, since model contains 2 bits for ACK in FD frame.
             *      Insert Passive Error frame to driven frame (TX/RX feedback enabled).
             *************************************************************************************/

            if (test_variant == TestVariant::Common) {
                if (elem_test.index == 1 || elem_test.index == 2) {
                    BitValue bit_value;
                    if (elem_test.index == 1) {
                        bit_value = BitValue::Dominant;
                    } else {
                        bit_value = BitValue::Recessive;
                    }

                    InsertCrcError(ChooseCrcBitToCorrupt(driver_bit_frm.get(), bit_value));

                } else if (elem_test.index == 3) {
                    driver_bit_frm->GetBitOf(0, BitType::Sof)->bit_value_ = BitValue::Recessive;
                }

            } else if (test_variant == TestVariant::CanFdEnabled) {
                if (elem_test.index >= 1 && elem_test.index <= 4) {
                    BitValue bit_value;
                    if (elem_test.index == 1 || elem_test.index == 3) {
                        bit_value = BitValue::Dominant;
                    } else {
                        bit_value = BitValue::Recessive;
                    }

                    InsertCrcError(ChooseCrcBitToCorrupt(driver_bit_frm.get(), bit_value));

                } else if (elem_test.index == 5) {
                    // TODO: Do the shortening to test CRC Issue!

                } else if (elem_test.index == 6) {
                    driver_bit_frm->GetBitOf(0, BitType::StuffParity)->FlipBitValue();
                    // Stuff bit post stuff parity is inserted with same field type!
                    driver_bit_frm->GetBitOf(1, BitType::StuffParity)->FlipBitValue();
                }
            }

            monitor_bit_frm->TurnReceivedFrame();
            monitor_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Eof);
            driver_bit_frm->InsertPassiveErrorFrame(0, BitType::Eof);

            // TODO: Skip test for CRC issue, will be finished later!
            if (elem_test.index == 5)
                return FinishElementaryTest();

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckNoRxFrame();

            FreeTestObjects();
            return FinishElementaryTest();
        }
        
        ENABLE_UNUSED_ARGS
};
