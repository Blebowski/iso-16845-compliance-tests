/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.3.1
 *
 * @brief This test verifies that an IUT acting as a transmitter tolerates up
 *        to 7 dominant bits after sending its own error flag.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *          FDF = 0
 *      CAN FD Enabled
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 the LT extends the error flag by 1 dominant bit;
 *          #2 the LT extends the error flag by 4 dominant bits;
 *          #3 the LT extends the error flag by 7 dominant bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. The LT corrupts this frame in
 *  data field causing the IUT to send an active error frame. The LT prolongs
 *  the error flag sent by IUT according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate only one error frame.
 *  The IUT shall restart the transmission after the intermission field
 *  following the error frame.
 * 
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

class TestIso_8_3_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80; // 7-th data bit will be recessive stuff bit
            if (test_variant == TestVariant::Common)
                frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, RtrFlag::DataFrame);
            else
                frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, EsiFlag::ErrorActive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Turn driven frame as received.
             *  2. Flip 7-th data bit of driven frame to dominant, this will destroy recessive
             *     stuff bit send by IUT.
             *  3. Insert expected active error frame from 8-th bit of data field to monitored
             *     frame. Insert the same to driven frame.
             *  4. Insert 1,4,7 dominant bits to driven frame after active error flag in driven
             *     frame (prolong error flag). Insert equal amount of recessive bits to monitored
             *     frame (this corresponds to accepting longer Error flag without re-sending next
             *     error flag).
             *  5. Append the same frame second time. This checks retransmission.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->bit_value_ = BitValue::Dominant;

            int bit_index = driver_bit_frm->GetBitIndex(
                driver_bit_frm->GetBitOf(7, BitType::Data));
            driver_bit_frm->InsertActiveErrorFrame(bit_index);
            monitor_bit_frm->InsertActiveErrorFrame(bit_index);

            int bits_to_insert;
            if (elem_test.index == 1)
                bits_to_insert = 1;
            else if (elem_test.index == 2)
                bits_to_insert = 4;
            else
                bits_to_insert = 7;

            Bit *first_err_delim_bit = driver_bit_frm->GetBitOf(0, BitType::ErrorDelimiter);
            int first_err_delim_index = driver_bit_frm->GetBitIndex(first_err_delim_bit);

            for (int k = 0; k < bits_to_insert; k++)
            {
                driver_bit_frm->InsertBit(BitType::ActiveErrorFlag, BitValue::Dominant, first_err_delim_index);
                monitor_bit_frm->InsertBit(BitType::PassiveErrorFlag, BitValue::Recessive, first_err_delim_index);
            }

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
    
};