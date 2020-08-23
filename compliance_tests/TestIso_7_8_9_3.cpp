/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.9.3
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        CRC delimiter.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          CRC delimiter
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the CRC delimiter bit to dominant from the second
 *             TQ until the beginning of Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT generates a frame with last CRC bit dominant.
 *  The LT forces the CRC delimiter bit to dominant according to elementary
 *  test cases.
 *
 * Response:
 *  The modified CRC delimiter bit shall be sampled as dominant.
 *  The frame is invalid. The CRC delimiter shall be followed by an error
 *  frame.
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

class TestIso_7_8_9_3 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift, Base ID only and
            uint8_t dataByte = 0x49;
            FrameFlags frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                RtrFlag::DataFrame, BrsFlag::Shift,
                                                EsiFlag::ErrorActive);
            // Frame was empirically debugged to have last bit of CRC in 1!
            golden_frame = new Frame(frameFlags, 0x1, 50, &dataByte);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("DontShift synchronisation after dominant bit sampled on CRC delimiter bit!");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force CRC delimiter bit to dominant from 2nd TQ till beginning
             *      of Phase Segment 2.
             *   3. Insert Active error frame to monitor from ACK bit further.
             *      Insert Passive error frame to driver bit from ACK bit further.
             */
            monitor_bit_frame->TurnReceivedFrame();

            Bit *crcDelimiter = driver_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
            crcDelimiter->ForceTimeQuanta(1, data_bit_timing.ph1_ + data_bit_timing.prop_,
                                          BitValue::Dominant);

            driver_bit_frame->InsertPassiveErrorFrame(
                driver_bit_frame->GetBitOf(0, BitType::Ack));
            monitor_bit_frame->InsertActiveErrorFrame(
                monitor_bit_frame->GetBitOf(0, BitType::Ack));

            driver_bit_frame->Print(true);
            monitor_bit_frame->Print(true);

            // Push frames to Lower tester, run and check!
            PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            DeleteCommonObjects();

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};