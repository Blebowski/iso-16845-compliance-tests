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
 * @test ISO16845 7.8.8.3
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there is an additional recessive
 *        to dominant edge between two sample points where the first edge comes
 *        before the synchronization segment on bit position ACK.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Bit start with negative offset and glitch between synchronization
 *      segment and sample point.
 *          ACK
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *      #1 The LT reduce the length of CRC delimiter bit by one TQ(D) and the
 *         LT force the second TQ of ACK bit to Recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *  Additionally, the Phase_Seg2(N) of this dominant ACK bit shall be forced
 *  to recessive.
 *
 * Response:
 *  The modified ACK bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
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

class TestIso_7_8_8_3 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Note: In this TC TX to RX feedback cant be enabled, since DUT
            //       would corrupt test pattern by IUT in ACK field!

            // CAN FD enabled only!
            if (dut_can_version == CanVersion::Can_2_0 ||
                dut_can_version == CanVersion::CanFdTolerant)
            {
                test_result = false;
                return false;
            }

            // CAN FD frame with bit rate shift
            FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift);
            golden_frame = new Frame(frameFlags);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("Testing ACK bit glitch filtering on negative phase error");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten CRC delimiter by 1 TQ in both driven and monitored
             *      frames.
             *   3. Force 2nd TQ of driven ACK bit to Recessive.
             *   4. Force whole Phase 2 of ACK bit to Recessive.
             */
            monitor_bit_frame->TurnReceivedFrame();

            Bit *crcDelimDriver = driver_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
            Bit *crcDelimMonitor = monitor_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
            Bit *ackBit = driver_bit_frame->GetBitOf(0, BitType::Ack);

            // ACK must be sent dominant since TX/RX feedback is not turned on!
            ackBit->bit_value_ = BitValue::Dominant;

            crcDelimDriver->ShortenPhase(BitPhase::Ph2, 1);
            crcDelimMonitor->ShortenPhase(BitPhase::Ph2, 1);

            ackBit->ForceTimeQuanta(1, BitValue::Recessive);
            ackBit->ForceTimeQuanta(0, nominal_bit_timing.ph2_ - 1,
                                    BitPhase::Ph2, BitValue::Recessive);

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

            DeleteCommonObjects();

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};