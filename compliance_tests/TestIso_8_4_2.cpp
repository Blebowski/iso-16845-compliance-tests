/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 8.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.2
 *
 * @brief This test verifies that an IUT acting as a transmitter generates an
 *        overload frame when it detects a dominant bit on the eighth bit of
 *        an error or an overload delimiter.
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
 *          #1 dominant bit on the eighth bit of an error delimiter, error
 *             applied in data field;
 *          #2 dominant bit on the eighth bit of an overload delimiter
 *             following a data frame.
 * 
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an error frame or overload frame
 *  according to elementary test cases.
 *  Then, the LT forces the eighth bit of the delimiter to a dominant state.
 * 
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the dominant bit generated by the LT.
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

class TestIso_8_4_2 : public test_lib::TestBase
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

            // Set TEC, so that IUT becomes error passive. Keep sufficient
            // reserve from 128 for decrements due to test frames!
            dut_ifc->SetTec(200);

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

                for (int j = 0; j < 2; j++)
                {
                    FrameFlags frameFlags;

                    if (i == 0)
                        frameFlags = FrameFlags(FrameType::Can2_0, RtrFlag::DataFrame);
                    else
                        frameFlags = FrameFlags(FrameType::CanFd, EsiFlag::ErrorPassive);

                    uint8_t dataByte = 0x80;
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
                     *  2. In first elementary test, force 7-th data bit
                     *     (should be recessive stuff bit) to dominant. In
                     *     second elementary test, force first bit of intermission
                     *     to dominant. Insert Error Frame (first elementary test)
                     *     or Overload frame (second elementary test) from next
                     *     bit on monitored frame. Insert passive Error frame
                     *     also to driven frame.
                     *  3. Force last bit of Error delimiter (first elementary test),
                     *     Overload delimiter (second elementary test) to dominant.
                     *  4. Insert Overload frame from next bit on monitored frame.
                     *     Insert Passive Error frame on driven frame so that LT does
                     *     not affect IUT.
                     *  5. Insert 8-more bits after intermission (behin 2-nd overload
                     *     frame). This emulates suspend transmission.
                     *  6. In first elemenary test, append the same frame after first
                     *     frame because frame shall be retransmitted (due to error
                     *     frame). This frame should immediately follow last bit of
                     *     suspend. In second elementary test, frame shall not be
                     *     re-transmitted, because there were only overload frames, so
                     *     append only dummy bits to check that unit does not retransmitt
                     *     (there were only overload frames)!
                     */
                    driver_bit_frame->TurnReceivedFrame();

                    if (j == 0)
                    {
                        Bit *dataStuffBit = driver_bit_frame->GetBitOf(6, BitType::Data);
                        dataStuffBit->bit_value_ = BitValue::Dominant;
                        monitor_bit_frame->InsertPassiveErrorFrame(
                            monitor_bit_frame->GetBitOf(7, BitType::Data));
                        driver_bit_frame->InsertPassiveErrorFrame(
                            driver_bit_frame->GetBitOf(7, BitType::Data));
                    }
                    else
                    {
                        Bit *firstIntermission = driver_bit_frame->GetBitOf(0, BitType::Intermission);
                        firstIntermission->bit_value_ = BitValue::Dominant;
                        monitor_bit_frame->InsertOverloadFrame(
                            monitor_bit_frame->GetBitOf(1, BitType::Intermission));
                        driver_bit_frame->InsertPassiveErrorFrame(
                            driver_bit_frame->GetBitOf(1, BitType::Intermission));
                    }

                    Bit *lastDelimBit = driver_bit_frame->GetBitOf(7, BitType::ErrorDelimiter);
                    int lastDelimIndex = driver_bit_frame->GetBitIndex(lastDelimBit);
                    lastDelimBit->bit_value_ = BitValue::Dominant;

                    monitor_bit_frame->InsertOverloadFrame(lastDelimIndex + 1);
                    driver_bit_frame->InsertPassiveErrorFrame(lastDelimIndex + 1);

                    // In second elementary test, last intermission bit is actually fourth
                    // intermission bit, because there is single bit of intermission before
                    // first error/overload fram!
                    int lastIntermissionIndex;
                    if (j == 0)
                        lastIntermissionIndex = 2;
                    else
                        lastIntermissionIndex = 3;

                    int endOfIntermissionIndex = driver_bit_frame->GetBitIndex(
                        driver_bit_frame->GetBitOf(lastIntermissionIndex, BitType::Intermission));
                    for (int k = 0; k < 8; k++)
                    {
                        driver_bit_frame->InsertBit(Bit(BitType::Suspend, BitValue::Recessive,
                            &frameFlags, &nominal_bit_timing, &data_bit_timing), endOfIntermissionIndex);
                        monitor_bit_frame->InsertBit(Bit(BitType::Suspend, BitValue::Recessive,
                            &frameFlags, &nominal_bit_timing, &data_bit_timing), endOfIntermissionIndex);
                    }

                    if (j == 0){
                        secondDriverBitFrame->TurnReceivedFrame();
                        driver_bit_frame->AppendBitFrame(secondDriverBitFrame);
                        monitor_bit_frame->AppendBitFrame(secondMonitorBitFrame);
                    } else {
                        for (int k = 0; k < 15; k++)
                        {
                            driver_bit_frame->InsertBit(
                                Bit(BitType::Idle, BitValue::Recessive,
                                    &frameFlags, &nominal_bit_timing, &data_bit_timing),
                                endOfIntermissionIndex);
                            monitor_bit_frame->InsertBit(
                                Bit(BitType::Idle, BitValue::Recessive,
                                    &frameFlags, &nominal_bit_timing, &data_bit_timing),
                                endOfIntermissionIndex);
                        }
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