/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.20
 * 
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when detecting a dominant bit at the last bit of an overload delimiter
 *        it is transmitting.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 * 
 *  CAN FD Enabled
 *      REC, FDF = 1
 * 
 * Elementary test cases:
 *      #1 It corrupts the last bit of the overload delimiter.
 *
 * Setup:
 *  DontShift action required, the IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  Then LT applies an error according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be zero.
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

class TestIso_7_6_20 : public test_lib::TestBase
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
            int rec, recNew;
            FrameType dataRate;

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

                // CAN 2.0 / CAN FD, randomize others
                FrameFlags frameFlags = FrameFlags(dataRate);
                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Forcing last bit of EOF to dominant!");

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received, insert ACK to driven frame.
                 *   2. Force last bit of EOF to DOMINANT.
                 *   3. Insert expected overload frame from first bit of Intermission.
                 *   4. Force 8-th bit of overload delimiter to dominant!
                 *   5. Insert next expected overload frame from first bit of
                 *      Intermission
                 */
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                driver_bit_frame->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

                monitor_bit_frame->InsertOverloadFrame(
                    monitor_bit_frame->GetBitOf(0, BitType::Intermission));
                driver_bit_frame->InsertOverloadFrame(
                    driver_bit_frame->GetBitOf(0, BitType::Intermission));

                driver_bit_frame->GetBitOf(7, BitType::OverloadDelimiter)->bit_value_ =
                    BitValue::Dominant;

                monitor_bit_frame->InsertOverloadFrame(
                    monitor_bit_frame->GetBitOf(0, BitType::Intermission));
                driver_bit_frame->InsertOverloadFrame(
                    driver_bit_frame->GetBitOf(0, BitType::Intermission));

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

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

                // Check that REC was not incremented
                recNew = dut_ifc->GetRec();
                if (recNew != 0)
                {
                    TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                    0, recNew);
                    test_result = false;
                    TestControllerAgentEndTest(test_result);
                    return test_result;
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