/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.9
 * 
 * @brief This test verifies the behaviour of the IUT when receiving two
 *        consecutive frames not separated by a bus idle state.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Intermission field length
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      Intermission field length
 *      FDF = 1
 * 
 * Elementary test cases:
 *      #1 The second frame starts after the second intermission bit of the
 *         first frame.
 *      #2 The second frame starts after the third intermission bit of the
 *         first frame.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  Two different test frames are used for each of the two elementary tests.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frames.
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

class TestIso_7_1_9 : public test_lib::TestBase
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
             * Common part of test
             ****************************************************************/
            TestMessage("Common part of test!");

            for (int i = 0; i < 2; i++)
            {
                TestBigMessage("\n\nIteration nr: %d\n", i + 1);

                // Generate 2 consecutivve frames
                // (Set CAN 2.0, randomize other)
                FrameFlags frameFlags = FrameFlags(FrameType::Can2_0);
                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame 1:");
                golden_frame->Print();

                FrameFlags frameFlags2 = FrameFlags(FrameType::Can2_0);
                goldenFrame2 = new Frame(frameFlags2);
                goldenFrame2->Randomize();
                TestBigMessage("Test frame 2:");
                goldenFrame2->Print();

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                driverBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitorBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                // In first iteration, Intermission lasts only 2 bits ->
                // Remove last bit of intermission!
                if (i == 0)
                {
                    printf("Removing bit!\n");
                    driver_bit_frame->RemoveBit(driver_bit_frame->GetBitOf(2, BitType::Intermission));
                    monitor_bit_frame->RemoveBit(monitor_bit_frame->GetBitOf(2, BitType::Intermission));
                    printf("Removed bit!\n");
                }

                // Monitor frames as if received, driver frame must have ACK too!
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                monitorBitFrame2->TurnReceivedFrame();
                driverBitFrame2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                PushFramesToLowerTester(*driverBitFrame2, *monitorBitFrame2);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                // Read received frames from DUT and compare with sent frames
                Frame readFrame = this->dut_ifc->ReadFrame();
                Frame readFrame2 = this->dut_ifc->ReadFrame();
                if (CompareFrames(*golden_frame, readFrame) == false ||
                    CompareFrames(*goldenFrame2, readFrame2) == false)
                {
                    test_result = false;
                    TestControllerAgentEndTest(test_result);
                }

                DeleteCommonObjects();

                if (test_result == false)
                    return false;
            }

            /*****************************************************************
             * CAN FD Enabled part of test!
             ****************************************************************/
            if (dut_can_version == CanVersion::CanFdEnabled)
            {
                TestMessage("CAN FD Enabled part of test!");
                for (int i = 0; i < 2; i++)
                {
                    TestBigMessage("\n\nIteration nr: %d\n", i + 1);

                    // Generate 2 consecutivve frames
                    // (Set CAN FD, randomize other)
                    FrameFlags frameFlags = FrameFlags(FrameType::CanFd);
                    golden_frame = new Frame(frameFlags);
                    golden_frame->Randomize();
                    TestBigMessage("Test frame 1:");
                    golden_frame->Print();

                    FrameFlags frameFlags2 = FrameFlags(FrameType::CanFd);
                    goldenFrame2 = new Frame(frameFlags2);
                    goldenFrame2->Randomize();
                    TestBigMessage("Test frame 2:");
                    goldenFrame2->Print();

                    // Convert to Bit frames
                    driver_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitor_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    driverBitFrame2 = new BitFrame(*goldenFrame2,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitorBitFrame2 = new BitFrame(*goldenFrame2,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    // In first iteration, Intermission lasts only 2 bits ->
                    // Remove last bit of intermission!
                    if (i == 0)
                    {
                        printf("Removing bit!\n");
                        driver_bit_frame->RemoveBit(driver_bit_frame->GetBitOf(2, BitType::Intermission));
                        monitor_bit_frame->RemoveBit(monitor_bit_frame->GetBitOf(2, BitType::Intermission));
                        printf("Removed bit!\n");
                    }

                    // Monitor frames as if received, driver frame must have ACK too!
                    monitor_bit_frame->TurnReceivedFrame();
                    driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                    monitorBitFrame2->TurnReceivedFrame();
                    driverBitFrame2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    // Push frames to Lower tester, run and check!
                    PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                    PushFramesToLowerTester(*driverBitFrame2, *monitorBitFrame2);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    // Read received frames from DUT and compare with sent frames
                    Frame readFrame = this->dut_ifc->ReadFrame();
                    Frame readFrame2 = this->dut_ifc->ReadFrame();
                    if (CompareFrames(*golden_frame, readFrame) == false ||
                        CompareFrames(*goldenFrame2, readFrame2) == false)
                    {
                        test_result = false;
                        TestControllerAgentEndTest(test_result);
                    }

                    delete golden_frame;
                    delete driver_bit_frame;
                    delete monitor_bit_frame;

                    if (test_result == false)
                        return false;
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