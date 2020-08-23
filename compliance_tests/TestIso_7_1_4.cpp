/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 3.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.4
 * 
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid base
 *        format frame.
 * 
 * @version CAN FD Enabled, Classical CAN
 * 
 * Test variables:
 *  Classical CAN  : FDF = 1
 *  CAN FD Enabled : FDF = 1, RRS = 1
 * 
 * Elementary test cases:
 *  Classical CAN:
 *      #1 FDF = 1
 * 
 *  CAN FD Enabled:
 *      #2 RRS = 1
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test cases.
 * 
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 * 
 * @todo: Classical CAN version not supported!
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

class TestIso_7_1_4 : public test_lib::TestBase
{
    public:

        int Run()
        {
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            /*****************************************************************
             * Classical CAN part
             ****************************************************************/
            if (dut_can_version == CanVersion::Can_2_0)
            {
                TestMessage("Classical CAN part of test not supporetd!");
                TestControllerAgentEndTest(test_result);
                // TODO: Add support for it (Protocol exception is done so should be OK)!
                return false;
            }

            /*****************************************************************
             * CAN FD Enabled part
             ****************************************************************/
            if (dut_can_version == CanVersion::CanFdEnabled)
            {
                TestMessage("CAN FD ENABLED part of test");

                // Generate frame (Set Base ID, Data frame, randomize others)
                FrameFlags frameFlagsFd = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                     RtrFlag::DataFrame);
                golden_frame = new Frame(frameFlagsFd);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                // Convert to bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                // Force RRS bit to recessive, update frames (Stuff bits and CRC
                // might change)!
                driver_bit_frame->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                monitor_bit_frame->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;

                // Update frames (Stuff bits, CRC might have changed!)
                driver_bit_frame->UpdateFrame();
                monitor_bit_frame->UpdateFrame();

                // Monitor frame as if received, driver frame must have ACK too!
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                // Read received frame from DUT and compare with sent frame
                Frame readFrame = this->dut_ifc->ReadFrame();
                if (CompareFrames(*golden_frame, readFrame) == false)
                {
                    test_result = false;
                    TestControllerAgentEndTest(test_result);
                }

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