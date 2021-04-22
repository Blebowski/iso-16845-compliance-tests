/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.8
 * 
 * @brief This test verifies that the IUT detects a form error when a fixed
 *        stuff bit did not match to the previous bit.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *      CAN FD Enabled:
 *          DLC - to cause different CRC types
 *          FDF = 1
 * 
 * Elementary test cases:
 *  There are 22 elementary tests to perform:
 *    Tests to perform on recessive stuff bits:
 *      #1 DLC ≤ 10 − > CRC (17) field – (6 bits)
 *      #2 DLC > 10 − > CRC (21) field – (7 bits)
 *    Tests to perform on dominant stuff bits:
 *      #3 DLC ≤ 10 − > CRC (17) field – (6 bits)
 *      #4 DLC > 10 − > CRC (21) field – (7 bits)
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT corrupts a fixed stuff bit according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame at the bit position following the
 *  stuff bit.
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

class TestIso_7_2_8 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);

            for (int i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc;
            BitValue bit_value;

            /* Tests 1,3 -> DLC < 10. Tests 2,4 -> DLC > 10 */
            if (elem_test.index % 2 == 0)
                dlc = (rand() % 5) + 0xA;
            else
                dlc = rand() % 10;

            if (elem_test.index < 3)
                bit_value = BitValue::Recessive;
            else
                bit_value = BitValue::Dominant;

            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, RtrFlag::DataFrame);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitored frame to received.
             *   2. Pick one of the stuff bits with required value (can be only in CRC field or
             *      stuff count!) and flip its value.
             *   3. Insert Active Error frame to monitored frame. Insert Passive Error frame to
             *      driven frame (TX/RX feedback enabled).
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            int num_stuff_bits = driver_bit_frm->GetNumStuffBits(StuffBitType::FixedStuffBit,
                                                                    bit_value);

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

                /* Skip stuff bits whose value is not matching */
                Bit *stuff_bit_to_flip = driver_bit_frm_2->GetFixedStuffBit(
                                            stuff_bit, bit_value);

                int bit_index = driver_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                stuff_bit_to_flip->FlipBitValue();

                driver_bit_frm_2->InsertPassiveErrorFrame(bit_index + 1);
                monitor_bit_frm_2->InsertActiveErrorFrame(bit_index + 1);

                driver_bit_frm_2->Print(true);
                monitor_bit_frm_2->Print(true);

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
