/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 24.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.2.7
 *
 * @brief This test verifies the behaviour in the CRC delimiter and acknowledge
 *        field when these fields are extended to 2 bits.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      CRC delimiter
 *      ACK slot
 *      FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 CRC delimiter up to 2-bit long (late ACK bit – long distance);
 *          #2 ACK up to 2-bit long (superposing ACK bits – near and long distance).
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT creates a CRC
 *  delimiter and an ACK bit as defined in elementary test cases.
 *
 * Response:
 *  The frame is valid. The IUT shall not generate an error frame.
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

class TestIso_8_2_7 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Start monitoring when DUT starts transmitting!
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));

            // Configure driver to wait for monitor so that LT sends ACK in right moment.
            CanAgentSetWaitForMonitor(true);

            // Enable TX/RX feedback so that DUT will see its own transmitted frame!
            CanAgentConfigureTxToRxFeedback(true);

            // For CAN FD enabled nodes only!
            if (dut_can_version != CanVersion::CanFdEnabled)
                return false;

            /*****************************************************************
             * CRC Delimiter 2 bit long (i = 0), ACK bit 2 bit long (i = 1)
             ****************************************************************/
            for (int i = 0; i < 2; i++)
            {
                if (i == 0)
                    TestMessage("Testing 2 bit long CRC delimiter!");
                else
                    TestMessage("Testing 2 bit long ACK!");

                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, EsiFlag::ErrorActive);

                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn driven frame as received. Force ACK dominant.
                 *   2. For 2 bit CRC delimiter:
                 *          Insert one extra CRC delimiter bit to both driven
                 *          and monitored frame. This emulates late ACK.
                 *      For 2 bit ACK delimiter:
                 *          Insert one extra ACK bit to both driven and monitored
                 *          frame.
                 */
                driver_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                if (i == 0)
                {
                    Bit *crcDelimDriver = driver_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
                    Bit *crcDelimMonitor = monitor_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
                    int crcDelimIndex = driver_bit_frame->GetBitIndex(crcDelimDriver);

                    // Copy CRC delimiter
                    // Note: All nominal bit timing will be used in copied bit!
                    driver_bit_frame->InsertBit(*crcDelimDriver, crcDelimIndex);
                    monitor_bit_frame->InsertBit(*crcDelimMonitor, crcDelimIndex);
                } else {
                    Bit *ackDriver = driver_bit_frame->GetBitOf(0, BitType::Ack);
                    Bit *ackMonitor = monitor_bit_frame->GetBitOf(0, BitType::Ack);
                    int ackIndex = driver_bit_frame->GetBitIndex(ackDriver);

                    // Copy ACK bit
                    driver_bit_frame->InsertBit(*ackDriver, ackIndex);
                    monitor_bit_frame->InsertBit(*ackMonitor, ackIndex);
                }

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, insert to DUT, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                StartDriverAndMonitor();

                TestMessage("Sending frame via DUT!");
                this->dut_ifc->SendFrame(golden_frame);
                TestMessage("Sent frame via DUT!");
                
                WaitForDriverAndMonitor();
                CheckLowerTesterResult();

                DeleteCommonObjects();
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};