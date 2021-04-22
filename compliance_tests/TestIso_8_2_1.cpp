/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 28.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.2.1
 * 
 * @brief This test verifies that the IUT detects a bit error when the bit it
 *        is transmitting in a base format frame is different with the bit it
 *        receives.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Each frame field with exception of the arbitration field where only
 *      dominant bits shall be modified and the ACK slot that will not be tested.
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      Each frame field with exception of the arbitration field where only
 *      dominant bits shall be modified and the ACK slot that will not be tested.
 *      DLC - to cause different CRC types
 *      FDF = 1
 * 
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *   The test shall, at minimum, modify at least 1 dominant and 1 recessive bit
 *   in each field of the frame except for the arbitration field for which only
 *   dominant bits shall be modified. The ACK slot is not tested.
 * 
 *  CAN FD enabled
 *   The test shall, at minimum, modify at least 1 dominant and 1 recessive bit
 *   in each field of the frame except of the arbitration field for where only
 *   dominant bits shall be modified. The ACK slot is not tested.
 * 
 *   Bit error in a fix stuff bit for CRC (17) and CRC (21) + bit error in
 *   CRC (17) and CRC (21).
 * 
 *   There are 21 elementary tests to perform.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit the frames and creates a bit error
 *  according to elementary test cases.
 *  
 * Response:
 *  The IUT shall generate an active error frame starting at the bit position
 *  following the corrupted bit.
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

class TestIso_8_2_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 13; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
            for (int i = 0; i < 21; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            /* TX to RX feedback must be disabled since we corrupt dominant bits to Recessive */
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* Choose frame field per elementary test */
            BitField bit_field_to_corrupt = BitField::Sof;
            BitValue bit_value_to_corrupt = BitValue::Dominant;
            switch (elem_test.index)
            {
            case 1:
                bit_field_to_corrupt = BitField::Sof;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            case 2:
                bit_field_to_corrupt = BitField::Arbitration;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            case 3:
                bit_field_to_corrupt = BitField::Control;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            case 4:
                bit_field_to_corrupt = BitField::Control;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            case 5:
                bit_field_to_corrupt = BitField::Data;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            case 6:
                bit_field_to_corrupt = BitField::Data;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            case 7:
                bit_field_to_corrupt = BitField::Crc;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            case 8:
                bit_field_to_corrupt = BitField::Crc;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            case 9:
                bit_field_to_corrupt = BitField::Ack;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            case 10:
                bit_field_to_corrupt = BitField::Eof;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            
            /* 
             * Following are in both test variants. I dont know what ISO means,
             * so I chose some random!
             */
            case 11:
                bit_field_to_corrupt = BitField::Data;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            case 12:
                bit_field_to_corrupt = BitField::Data;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            case 13:
                bit_field_to_corrupt = BitField::Control;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            
            /*
             * Following are in CAN FD variant only! These are all in CRC!
             */
            case 14:
            case 15:
            case 16:
            case 17:
                bit_field_to_corrupt = BitField::Crc;
                bit_value_to_corrupt = BitValue::Recessive;
                break;
            case 18:
            case 19:
            case 20:
            case 21:
                bit_field_to_corrupt = BitField::Crc;
                bit_value_to_corrupt = BitValue::Dominant;
                break;
            default:
                break;
            }

            /* Choose dlc based on elementary test */
            uint8_t dlc;
            if (elem_test.index < 14) {
                dlc = (rand() % 7) + 1; /* To make sure at least 1! */
            } else {
                /* Distribute DLC so that following elementary tests get CRC17 */
                if (elem_test.index == 14 || elem_test.index == 15 ||
                    elem_test.index == 18 || elem_test.index == 19)
                    dlc = 0x8;
                else
                    dlc = 0xC;
            }

            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::Shift, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* Second frame the same due to retransmission. */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);                    
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame so that IUT does not detect ACK error!
             *   2. Choose randomi bit within bit field as given by elementary test
             *   3. Corrupt value of this bit in driven frame.
             *   4. Insert Active Error flag from next bit on in both driven and monitored frames!
             *   5. Append the same frame after first frame as if retransmitted by IUT!
             *************************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            /* Choose random Bit type within some bit field */
            BitType bit_type = GetRandomBitType(elem_test.frame_type, IdentifierType::Base,
                                                bit_field_to_corrupt);

            /* Search for bit of matching value! */
            int lenght = driver_bit_frm->GetFieldLength(bit_type);
            int index_in_bitfield = rand() % lenght;
            Bit *bit_to_corrupt = driver_bit_frm->GetBitOf(index_in_bitfield, bit_type);

            /* In following elementary tests we aim for fixed stuff bit of this value!
             * We should have it guaranteed that all following combinations are tested:
             *  [Recessive, Dominant] x [CRC17, CRC21] x [Normal, Fixed Stuff bit]
             */
            if (elem_test.index == 15 || elem_test.index == 16 ||
                elem_test.index == 19 || elem_test.index == 20)
            {
                int attempt_cnt = 0;
                while (bit_to_corrupt->bit_value_ != bit_value_to_corrupt ||
                       bit_to_corrupt->stuff_bit_type != StuffBitType::FixedStuffBit)
                {
                    bit_type = GetRandomBitType(elem_test.frame_type, IdentifierType::Base,
                                                bit_field_to_corrupt);
                    lenght = driver_bit_frm->GetFieldLength(bit_type);
                    index_in_bitfield = rand() % lenght;
                    bit_to_corrupt = driver_bit_frm->GetBitOf(index_in_bitfield, bit_type);
                    attempt_cnt++;

                    // Due to frame randomization, it can happend that we are searching for
                    // dominant fixed stuff bit, but there is no such bit in this frame.
                    // After 30 attempts, give up and corrupt last generated bti regardless
                    // of its value or fixed stuff bit properties.
                    if (attempt_cnt == 30)
                        break;
                }
            } else {
                /* Pick different Bit Type within bit field for each search! It can happend
                 * that initially, bit type will be picked which does not have bits of this
                 * value! This avoids getting stuck in searching for bit to corrupt!
                 */
                while (bit_to_corrupt->bit_value_ != bit_value_to_corrupt)
                {
                    bit_type = GetRandomBitType(elem_test.frame_type, IdentifierType::Base,
                                                bit_field_to_corrupt);
                    lenght = driver_bit_frm->GetFieldLength(bit_type);
                    index_in_bitfield = rand() % lenght;
                    bit_to_corrupt = driver_bit_frm->GetBitOf(index_in_bitfield, bit_type);
                }
            }


            

            TestMessage("Corrupting bit type: %s", bit_to_corrupt->GetBitTypeName().c_str());
            TestMessage("Index in bit field: %d", index_in_bitfield);
            TestMessage("Value to be corrupted: %d", (int)bit_to_corrupt->bit_value_);

            int bit_index = driver_bit_frm->GetBitIndex(bit_to_corrupt);
            bit_to_corrupt->FlipBitValue();

            driver_bit_frm->InsertActiveErrorFrame(bit_index + 1);
            monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetTec(0); /* Avoid turning error passive */
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }

};