/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 23.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.1.3
 * 
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *          CRC Delimiter
 *          FDF = 1
 * 
 * Elementary test cases:
 *      There are two elementary tests to perform for at least 1 bit rate
 *      configuration:
 *          #1 test for early sampling point: bit level change to recessive
 *             before sampling point;
 *          #2 test for late sampling point: bit level change to recessive
 *             after sampling point.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 * 
 *  Test CRC delimiter #1:
 *      The LT forces a recessive CRC delimiter bit to dominant from beginning
 *      up to one TQ(D) before the Sampling point.
 * 
 *  Test CRC delimiter #2:
 *      The LT forces a recessive CRC delimiter bit to dominant from beginning
 *      up to the sampling point.
 * 
 * Response:
 *  Test CRC delimiter #1:
 *      The modified CRC delimiter bit shall be sampled as recessive.
 *      The frame is valid. DontShift error flag shall occur.
 * 
 *  Test CRC delimiter #2:
 *      The modified CRC delimiter bit shall be sampled as dominant.
 *      The frame is invalid. An error frame shall follow.
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

class TestIso_7_8_1_3 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Enable TX to RX feedback
            CanAgentConfigureTxToRxFeedback(true);

            // CAN FD enabled only!
            if (dut_can_version == CanVersion::Can_2_0 ||
                dut_can_version == CanVersion::CanFdTolerant)
            {
                test_result = false;
                return false;
            }

            /*****************************************************************
             * CRC Delimiter sampled Recessive (OK) /
             * CRC Delimiter sampled Dominant (Error frame)
             ****************************************************************/
            for (int i = 0; i < 2; i++)
            {
                // CAN FD frame
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift);
                golden_frame = new Frame(frameFlags);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                if (i == 0)
                    TestMessage("Testing CRC delimiter bit sampled Recessive");
                else
                    TestMessage("Testing CRC delimiter bit sampled Dominant");

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Modify CRC Delimiter, flip TSEG1 - 1 (i == 0) or TSEG1
                 *      (i == 1) to dominant!
                 *   3. For i == 1, insert active error frame right after CRC
                 *      delimiter! Insert passive error frame to driver to send
                 *      all recessive (TX to RX feedback is turned ON)!
                 */
                monitor_bit_frame->TurnReceivedFrame();

                Bit *crcDelim = driver_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
                int bitIndex = driver_bit_frame->GetBitIndex(crcDelim);
                int domPulseLength;

                if (i == 0)
                    domPulseLength = data_bit_timing.prop_ + data_bit_timing.ph1_;
                else
                    domPulseLength = data_bit_timing.prop_ + data_bit_timing.ph1_ + 1;

                for (int j = 0; j < domPulseLength; j++)
                    crcDelim->ForceTimeQuanta(j, BitValue::Dominant);    

                if (i == 1)
                {
                    driver_bit_frame->InsertPassiveErrorFrame(bitIndex + 1);
                    monitor_bit_frame->InsertActiveErrorFrame(bitIndex + 1);
                }

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                // Read received frame from DUT and compare with sent frame
                // (for i==0 only, i==1 ends with error frame)
                Frame readFrame = this->dut_ifc->ReadFrame();
                if ((i == 0) && (CompareFrames(*golden_frame, readFrame) == false))
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