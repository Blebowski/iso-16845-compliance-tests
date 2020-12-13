/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 2.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.10
 * 
 * @brief This test verifies that the IUT increases its REC by 1 when detecting
 *        a CRC error during reception of a frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, ACK = 1 Bit recessive, FDF = 0
 * 
 *  CAN FD Enabled
 *      REC, DLC to cause different CRC types, ACK = 2 Bit recessive
 *      FDF = 1
 * 
 * Elementary test cases:
 *  Classical CAN, CAN FD tolerant, CAN FD enabled:
 *      There is one elementary test to perform:
 *          #1 CRC (15) error
 *  
 *  CAN FD enabled:
 *      Elementary tests to perform:
 *          #1 DLC ≤ 10 − > CRC (17) error
 *          #2 DLC > 10 − > CRC (21) error
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame containing an error according to elementary test
 *  cases.
 * 
 * Response:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      The IUT sends a recessive acknowledge.
 *      The IUT starts the transmission of an active error frame at the first
 *      bit position following the ACK delimiter.
 *      The IUT’s REC value shall be increased by 1 by starting the error
 *      frame.
 *  CAN FD enabled:
 *      The IUT sends a recessive acknowledge.
 *      The IUT starts the transmission of an active error frame at the fourth
 *      bit position following the CRC delimiter.
 *      The IUT’s REC value shall be increased by 1 by starting the error
 *      frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <bitset>

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

class TestIso_7_6_10 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            for (int i = 0; i < 2; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, RtrFlag::DataFrame);
            if (test_variant == TestVariant::Common)
            {
                golden_frm = std::make_unique<Frame>(*frame_flags);
            } else {
                uint8_t dlc;
                if (elem_test.index == 1)
                    dlc = rand() % 0xB;
                else
                    dlc = (rand() % 0x4) + 0xB;
                golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
            }
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received. Force ACK low in monitored frame since IUT shall
             *      not send ACK then!
             *   2. Choose random bit of CRC which is not stuff bit and flip its value.
             *      (TODO: This can have a problem in CRC15 if we modify a bit which is part of
             *             sequence of consecutive bits after which stuff bit is inserted! Then
             *             this can change IUTs interpretation of CRC field lenght).
             *   3. Insert Active Error flag from first bit of EOF.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            monitor_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

            int crc_bit_index;
            Bit *crc_bit;
            do {
                crc_bit_index = rand() % driver_bit_frm->GetFieldLength(BitType::Crc);
                crc_bit = driver_bit_frm->GetBitOf(crc_bit_index, BitType::Crc);
            } while (crc_bit->stuff_bit_type != StuffBitType::NoStuffBit);
            crc_bit->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(0, BitType::Eof);
            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Eof);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            
            CheckLowerTesterResult();
            CheckNoRxFrame();
            CheckRecChange(rec_old, +1);

            FreeTestObjects();
            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};