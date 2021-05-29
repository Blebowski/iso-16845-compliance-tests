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
 * @test ISO16845 8.3.4
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a form
 *        error when it receives an invalid error delimiter.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          FDF = 0
 *      CAN FD Enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 corrupting the second bit of the error delimiter;
 *          #2 corrupting the fourth bit of the error delimiter;
 *          #3 corrupting the seventh bit of the error delimiter.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT corrupts this frame in data field causing the IUT to send an active
 *  error frame.
 *  Then, the LT forces 1 recessive bit of the error delimiter to the dominant
 *  state according to elementary test cases.
 * 
 * Response:
 *  The IUT shall restart the error frame at the bit position following the
 *  corrupted bit.
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

class TestIso_8_3_4 : public test_lib::TestBase
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

            SetupMonitorTxTests();
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
             *  4. Flip 2,4 or 7-th bit of Error delimiter to Dominant. Insert next expected error
             *     frame from one bit further.
             *  5. Turn second driven frame (the same) as received. Append after first frame. This
             *     checks retransmission.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            int bit_index_to_flip;
            if (elem_test.index_ == 1)
                bit_index_to_flip = 2;
            else if (elem_test.index_ == 2)
                bit_index_to_flip = 4;
            else
                bit_index_to_flip = 7;

            Bit *bit_to_flip = driver_bit_frm->GetBitOf(bit_index_to_flip - 1,
                                                        BitType::ErrorDelimiter);
            driver_bit_frm->FlipBitAndCompensate(bit_to_flip, dut_input_delay);

            int next_error_flag_index = driver_bit_frm->GetBitIndex(bit_to_flip) + 1;

            driver_bit_frm->InsertActiveErrorFrame(next_error_flag_index);
            monitor_bit_frm->InsertActiveErrorFrame(next_error_flag_index);

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
            this->dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }

};