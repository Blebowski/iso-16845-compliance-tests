/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.3.2
 * 
 * @brief The purpose of this test is to verify that an IUT accepts a frame
 *        starting after the second bit of the intermission following the error
 *        frame it has transmitted.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      FDF = 1
 * 
 * Elementary test cases:
 *      #1 Frame is started 2 bits after the end of the error delimiter.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field.
 *  The LT sends a valid frame according to elementary test cases.
 * 
 * Response:
 *  The IUT shall acknowledge the test frame in data field.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
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

class TestIso_7_3_2 : public test_lib::TestBase
{
    public:

        Frame *goldenFrame2;
        BitFrame *driverBitFrame2;
        BitFrame *monitorBitFrame2;

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
            uint8_t dataByte = 0x80;

            if (dut_can_version == CanVersion::CanFdEnabled)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                {
                    TestMessage("Common part of test!");
                    dataRate = FrameType::Can2_0;
                } else {
                    TestMessage("CAN FD enabled part of test!");
                    dataRate = FrameType::CanFd;
                }

                // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
                // randomize Identifier 
                FrameFlags frameFlags = FrameFlags(dataRate, RtrFlag::DataFrame);
                golden_frame = new Frame(frameFlags, 1, &dataByte);
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
                 *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
                 *      This will cause stuff error!
                 *   3. Insert Active Error frame from 8-th bit of data frame!
                 *   4. Remove last bit of Intermission (after error frame)
                 *   5. Insert second frame directly after first frame.
                 */
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(6, BitType::Data)->FlipBitValue();

                monitor_bit_frame->InsertActiveErrorFrame(
                    monitor_bit_frame->GetBitOf(7, BitType::Data));
                driver_bit_frame->InsertActiveErrorFrame(
                    driver_bit_frame->GetBitOf(7, BitType::Data));

                driver_bit_frame->RemoveBit(driver_bit_frame->GetBitOf(2, BitType::Intermission));
                monitor_bit_frame->RemoveBit(monitor_bit_frame->GetBitOf(2, BitType::Intermission));

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Generate frame 2 - randomize everything
                FrameFlags frameFlags2 = FrameFlags();
                goldenFrame2 = new Frame(frameFlags);
                goldenFrame2->Randomize();
                TestBigMessage("Test frame 2:");
                goldenFrame2->Print();

                driverBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitorBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitorBitFrame2->TurnReceivedFrame();
                driverBitFrame2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                PushFramesToLowerTester(*driverBitFrame2, *monitorBitFrame2);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                // Read out frame from DUT and check it!
                Frame readFrame = this->dut_ifc->ReadFrame();
                if (CompareFrames(*goldenFrame2, readFrame) == false)
                {
                    test_result = false;
                    TestControllerAgentEndTest(test_result);
                }

                DeleteCommonObjects();

                delete goldenFrame2;
                delete driverBitFrame2;
                delete monitorBitFrame2;
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};