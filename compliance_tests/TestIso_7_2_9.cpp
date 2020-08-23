/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 6.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.9
 * 
 * @brief This test verifies that the IUT detects a form error when the
 *        recessive ACK delimiter is forced to dominant state by LT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ACK delimiter, ACK = 0, FDF = 0
 * 
 *  CAN FD Enabled
 *      ACK delimiter, ACK1 = 0, ACK2 = 0, FDF = 1
 * 
 * Elementary test cases:
 *      #1 ACK delimiter = 0
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with form error at ACK delimiter according to
 *  elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an active error frame at the bit position following
 *  the ACK delimiter.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

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

class TestIso_7_2_9 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
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
                {
                    // Generate CAN frame (CAN 2.0, randomize others)
                    TestMessage("Common part of test!");
                    dataRate = FrameType::Can2_0;
                } else {
                    // Generate CAN frame (CAN FD, randomize others)
                    TestMessage("CAN FD enabled part of test!");
                    dataRate = FrameType::CanFd;
                }
                FrameFlags frameFlags = FrameFlags(dataRate);
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
                 *   1. Monitor frame as if received.
                 *   2. CAN 2.0 -> Force ACK Delimiter DOMINANT
                 *      CAN FD -> Force first bit of ACK to dominant
                 *             -> Add second ACK bit, driven dominant, monitored recessive
                 *   3. Insert Active Error frame from first bit of EOF!
                 */
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                driver_bit_frame->GetBitOf(0, BitType::AckDelimiter)->bit_value_ = BitValue::Dominant;

                if (dataRate == FrameType::CanFd)
                {
                    Bit *ackDelimBit = driver_bit_frame->GetBitOf(0, BitType::AckDelimiter);
                    int ackDelimIndex = driver_bit_frame->GetBitIndex(ackDelimBit);
                    driver_bit_frame->InsertBit(
                        Bit(BitType::Ack, BitValue::Dominant, &frameFlags,
                        &this->nominal_bit_timing, &this->data_bit_timing, StuffBitType::NoStuffBit),
                        ackDelimIndex);
                    monitor_bit_frame->InsertBit(
                        Bit(BitType::Ack, BitValue::Recessive, &frameFlags,
                        &this->nominal_bit_timing, &this->data_bit_timing, StuffBitType::NoStuffBit),
                        ackDelimIndex);
                }

                monitor_bit_frame->InsertActiveErrorFrame(
                    monitor_bit_frame->GetBitOf(0, BitType::Eof));
                driver_bit_frame->InsertActiveErrorFrame(
                    driver_bit_frame->GetBitOf(0, BitType::Eof));

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
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