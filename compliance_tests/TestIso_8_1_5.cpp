/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 12.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.5
 *
 * @brief The purpose of this test is to verify the point of time at which a
 *        message transmitted by the IUT is taken to be valid.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled - FDF = 0
 *      CAN FD enabled - FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 * 
 *      #1 On the first bit of the intermission field of the frame sent by the
 *         IUT, the LT forces the bit value to dominant.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a data frame. The LT causes the IUT to
 *  generate an overload frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall not restart any frame after the overload frame.
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

class TestIso_8_1_5 : public test_lib::TestBase
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
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/
            for (int i = 0; i < 2; i++)
            {
                // CAN 2.0 Frame, Base ID only, Data frame
                FrameFlags frameFlags;
                
                if (i == 0){
                    frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base,
                                            RtrFlag::DataFrame);
                } else {
                    frameFlags = FrameFlags(FrameType::CanFd, EsiFlag::ErrorActive);
                }

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
                 *   1. Turn driven frame as if received (insert ACK).
                 *   2. Force first bit of driven frame to dominant.
                 *   3. Insert overload flag from 2nd bit of intermission further!
                 *   4. Insert 15 recessive bits at the end of Overload delimiter.
                 *      This checks that DUT does not retransmitt the frame!
                 */
                driver_bit_frame->TurnReceivedFrame();

                driver_bit_frame->GetBitOf(0, BitType::Intermission)
                    ->bit_value_ = BitValue::Dominant;

                Bit *overloadStartBit = monitor_bit_frame->GetBitOf(1, BitType::Intermission);
                monitor_bit_frame->InsertOverloadFrame(overloadStartBit);

                Bit *overloadEndBit = monitor_bit_frame->GetBitOf(6, BitType::OverloadDelimiter);
                int bitIndex = monitor_bit_frame->GetBitIndex(overloadEndBit);
                for (int i = 0; i < 15; i++)
                    monitor_bit_frame->InsertBit(Bit(BitType::OverloadDelimiter, BitValue::Recessive,
                                                   &frameFlags, &nominal_bit_timing, &data_bit_timing),
                                                bitIndex);

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