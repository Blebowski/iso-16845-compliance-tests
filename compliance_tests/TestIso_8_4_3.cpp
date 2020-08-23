/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 9.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.3
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a data frame starting with the identifier and without transmitting
 *        SOF, when detecting a dominant bit on the third bit of the intermi-
 *        ssion field following an overload frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 0
 *      CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 * 
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits.
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT disturbs the transmitted frame with an error frame, then the LT causes
 *  the IUT to generate an overload frame immediately after the error frame.
 *  Then, the LT forces the third bit of the intermission following the overload
 *  delimiter to dominant state.
 * 
 * Response:
 *  The IUT shall repeat the frame starting with the identifier without
 *  transmitting any SOF.
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

class TestIso_8_4_3 : public test_lib::TestBase
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

                for (int j = 0; j < 2; j++)
                {
                    FrameFlags frameFlags;
                    uint8_t dataByte = 0x80;
                    int ids[] = {
                        0x7B,
                        0x3B
                    };

                    if (i == 0)
                        frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base,
                                                RtrFlag::DataFrame);
                    else
                        frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                EsiFlag::ErrorActive);

                    golden_frame = new Frame(frameFlags, 0x1, ids[j], &dataByte);
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
                     *  2. Force 7-th data bit to Dominant. This should be recessive
                     *     stuff bit. Insert Active Error frame from next bit on, to
                     *     monitored frame. Insert Passive Error frame to driven frame.
                     *  3. Force 8-th bit of Error delimiter to dominant. Insert Overload
                     *     frame from next bit on to monitored frame. Insert Passive
                     *     Error frame to driven frame.
                     *  4. Force third bit of intermission after overload frame to
                     *     dominant (in driven frame).
                     *  5. Remove SOF bit from retransmitted frame. Append retransmitted
                     *     frame behind the first frame. Second driven frame is turned
                     *     received.
                     */
                    driver_bit_frame->TurnReceivedFrame();

                    Bit *dataStuffBit = driver_bit_frame->GetBitOf(6, BitType::Data);
                    dataStuffBit->bit_value_ = BitValue::Dominant;
                    monitor_bit_frame->InsertActiveErrorFrame(
                        monitor_bit_frame->GetBitOf(7, BitType::Data));
                    driver_bit_frame->InsertPassiveErrorFrame(
                        driver_bit_frame->GetBitOf(7, BitType::Data));

                    Bit *endOfErrorDelim = driver_bit_frame->GetBitOf(7, BitType::ErrorDelimiter);
                    int endOfErrorDelimIndex = driver_bit_frame->GetBitIndex(endOfErrorDelim);
                    endOfErrorDelim->bit_value_ = BitValue::Dominant;
                    monitor_bit_frame->InsertOverloadFrame(endOfErrorDelimIndex + 1);
                    driver_bit_frame->InsertPassiveErrorFrame(endOfErrorDelimIndex + 1);

                    Bit *thirdIntermissionBit = driver_bit_frame->GetBitOf(2, BitType::Intermission);
                    thirdIntermissionBit->bit_value_ = BitValue::Dominant;

                    secondDriverBitFrame->TurnReceivedFrame();
                    secondDriverBitFrame->RemoveBit(secondDriverBitFrame->GetBitOf(0, BitType::Sof));
                    secondMonitorBitFrame->RemoveBit(secondMonitorBitFrame->GetBitOf(0, BitType::Sof));

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