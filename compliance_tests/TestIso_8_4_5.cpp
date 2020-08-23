/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 19.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.5
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a
 *        form error when it receives an invalid overload delimiter.
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
 * 
 *      Elementary tests to perform:
 *          #1 corrupting the second bit of the overload delimiter.
 *          #2 corrupting the fourth bit of the overload delimiter.
 *          #3 corrupting the seventh bit of the overload delimiter.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame.
 *  The LT corrupts the overload delimiter according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame starting at the bit position after
 *  the corrupted bit.
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

class TestIso_8_4_5 : public test_lib::TestBase
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

            CanAgentConfigureTxToRxFeedback(true);

            int iterCnt;

            if (dut_can_version == CanVersion::CanFdEnabled)
                iterCnt = 2;
            else
                iterCnt = 1;

            /*****************************************************************
             * Common part of test (i=0), CAN FD enabled part of test(i=1)
             ****************************************************************/
            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                    TestMessage("CAN 2.0 part of test");
                else
                    TestMessage("CAN FD part of test");

                for (int j = 0; j < 3; j++)
                {
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

                    BitFrame *secondDriverBitFrame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    BitFrame *secondMonitorBitFrame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    /**
                     * Modify test frames:
                     *  1. Turn driven frame as received.
                     *  2. Force first bit of Intermission to Dominant.
                     *     (Overload condition)
                     *  3. Insert Overload frame from second bit of Intermission to
                     *     monitored frame.
                     *  4. Force 2, 4, 7-th bit of Overload delimiter to Dominant.
                     *  5. Insert Passive Error frame from next bit to driven frame.
                     *     Insert Active Error frame to monitored frame.
                     *
                     *  Note: Don't insert retransmitted frame after first frame, since
                     *        error happened in overload frame which was transmitted
                     *        due to Overload condition in Intermission. At this point
                     *        frame has already been validated by transmitter! This is
                     *        valid according to spec. since for transmitter frame
                     *        vaidation shall occur at the end of EOF!
                     */
                    driver_bit_frame->TurnReceivedFrame();

                    Bit *firstIntermissionBit = driver_bit_frame->GetBitOf(0, BitType::Intermission);
                    firstIntermissionBit->bit_value_ = BitValue::Dominant;

                    driver_bit_frame->InsertOverloadFrame(
                        driver_bit_frame->GetBitOf(1, BitType::Intermission));
                    monitor_bit_frame->InsertOverloadFrame(
                        monitor_bit_frame->GetBitOf(1, BitType::Intermission));

                    Bit *bitToCorrupt;
                    if (j == 0){
                        bitToCorrupt = driver_bit_frame->GetBitOf(1, BitType::OverloadDelimiter);
                    } else if (j == 1){
                        bitToCorrupt = driver_bit_frame->GetBitOf(3, BitType::OverloadDelimiter);
                    } else {
                        bitToCorrupt = driver_bit_frame->GetBitOf(6, BitType::OverloadDelimiter);
                    }

                    int bitIndex = driver_bit_frame->GetBitIndex(bitToCorrupt);
                    bitToCorrupt->bit_value_ = BitValue::Dominant;

                    driver_bit_frame->InsertPassiveErrorFrame(bitIndex + 1);
                    monitor_bit_frame->InsertActiveErrorFrame(bitIndex + 1);

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
                    delete secondDriverBitFrame;
                    delete secondMonitorBitFrame;
                }
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};