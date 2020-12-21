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

class TestIso_8_3_1 : public test_lib::TestBase
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
                    uint8_t dataByte = 0x80; // 7-th data bit will be recessive stuff bit
                    FrameFlags frameFlags;
                    if (i == 0)
                        frameFlags = FrameFlags(FrameType::Can2_0, RtrFlag::DataFrame);
                    else
                        frameFlags = FrameFlags(FrameType::CanFd, EsiFlag::ErrorActive);

                    golden_frame = new Frame(frameFlags, 0x1, &dataByte);
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
                     *  2. Flip 7-th data bit of driven frame to dominant, this
                     *     will destroy recessive stuff bit send by IUT.
                     *  3. Insert expected active error frame from 8-th bit
                     *     of data field to monitored frame. Insert the same
                     *     to driven frame.
                     *  4. Insert 1,4,7 dominant bits to driven frame after active
                     *     error flag in driven frame (prolong error flag). Insert
                     *     equal amount of recessive bits to monitored frame (this
                     *     corresponds to accepting longer Error flag without
                     *     re-sending next error flag).
                     *  5. Append the same frame second time. This checks
                     *     retransmission.
                     */
                    driver_bit_frame->TurnReceivedFrame();
                    driver_bit_frame->GetBitOf(6, BitType::Data)->bit_value_ = BitValue::Dominant;

                    int bitIndex = driver_bit_frame->GetBitIndex(
                        driver_bit_frame->GetBitOf(7, BitType::Data));
                    driver_bit_frame->InsertActiveErrorFrame(bitIndex);
                    monitor_bit_frame->InsertActiveErrorFrame(bitIndex);

                    int bitsToInsert;
                    if (j == 0)
                        bitsToInsert = 1;
                    else if (j == 1)
                        bitsToInsert = 4;
                    else
                        bitsToInsert = 7;

                    Bit *firstErrDelimBit = driver_bit_frame->GetBitOf(0, BitType::ErrorDelimiter);
                    int firstErrDelimIndex = driver_bit_frame->GetBitIndex(firstErrDelimBit);

                    for (int k = 0; k < bitsToInsert; k++)
                    {
                        driver_bit_frame->InsertBit(BitType::ActiveErrorFlag, BitValue::Dominant, firstErrDelimIndex);
                        monitor_bit_frame->InsertBit(BitType::PassiveErrorFlag, BitValue::Recessive, firstErrDelimIndex);
                    }

                    secondDriverBitFrame->TurnReceivedFrame();
                    driver_bit_frame->AppendBitFrame(secondDriverBitFrame);
                    monitor_bit_frame->AppendBitFrame(secondMonitorBitFrame);

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
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};