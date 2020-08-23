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
 * @test ISO16845 8.2.6
 *
 * @brief This test verifies that the IUT detects an acknowledgement error when
 *        the received ACK slot is recessive.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          ACK Slot = 1 bit, FDF = 0
 *      CAN FD enabled:
 *          ACK Slot = 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          #1 ACK slot = recessive
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a base format frame. Then, the LT does
 *  not send the ACK slot according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame starting at the bit position
 *  following the ACK slot.
 *  The IUT shall restart the transmission of the frame as soon as the bus is
 *  idle.
 * 
 * Note:
 *  For classical format frame usage, the IUT shall generate an error frame
 *  starting at the bit position following the 1-bit wide ACK slot.
 *  For FD format frame, the IUT shall generate an error frame starting at the
 *  bit position following the 2-bit wide ACK slot.
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

class TestIso_8_2_6 : public test_lib::TestBase
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

            /*****************************************************************
             * Common part of test (i=0), CAN FD enabled part of test(i=1)
             ****************************************************************/
            int iterCnt;
            FrameType dataRate;

            if (dut_can_version == CanVersion::CanFdEnabled)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                    TestMessage("Common part of test!");
                else
                    TestMessage("CAN FD enabled part of test!");

                FrameFlags frameFlags;

                if (i == 0)
                    frameFlags = FrameFlags(FrameType::Can2_0);
                else
                    frameFlags = FrameFlags(FrameType::CanFd, EsiFlag::ErrorActive);

                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                BitFrame *secDriverBitFrame = new BitFrame(*golden_frame,
                                                &this->nominal_bit_timing, &this->data_bit_timing);
                BitFrame *secMonitorBitFrame = new BitFrame(*golden_frame,
                                                &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn driven frame as received, force ACK slot recessive
                 *      (to emulate missing ACK).
                 *   2. Insert expected error frame. In CAN 2.0, expected from
                 *      ACK Delimiter. In CAN FD. expect from EOF (as if ACK had
                 *      2 bits).
                 *   3. Append the same frame after the end of first one (to check
                 *      retransmission)
                 */
                driver_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

                if (i == 0)
                {
                    monitor_bit_frame->InsertActiveErrorFrame(
                        monitor_bit_frame->GetBitOf(0, BitType::AckDelimiter));
                    driver_bit_frame->InsertPassiveErrorFrame(
                        driver_bit_frame->GetBitOf(0, BitType::AckDelimiter));
                } else {
                    monitor_bit_frame->InsertActiveErrorFrame(
                        monitor_bit_frame->GetBitOf(0, BitType::Eof));
                    driver_bit_frame->InsertPassiveErrorFrame(
                        driver_bit_frame->GetBitOf(0, BitType::Eof));
                }

                secDriverBitFrame->TurnReceivedFrame();
                driver_bit_frame->AppendBitFrame(secDriverBitFrame);
                monitor_bit_frame->AppendBitFrame(secMonitorBitFrame);

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