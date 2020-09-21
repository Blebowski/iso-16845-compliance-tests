/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 16.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.2
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW(D) on bit position DATA.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          DATA field
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              |e| ∈ [1, SJW(D)]
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  The LT shortened the DATA bit before the dominant stuff bit by an amount
 *  of |e| TQ according to elementary test cases.
 *  Additionally, the Phase_Seg2(D) of the dominant stuff bit shall be forced
 *  to recessive.
 *
 * Response:
 *  The modified stuff bit shall be sampled as dominant.
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

class TestIso_7_8_5_2 : public test_lib::TestBase
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

            for (size_t i = 1; i <= data_bit_timing.sjw_; i++)
            {
                // CAN FD frame with bit rate shift
                uint8_t dataByte = 0x7F;
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift);
                golden_frame = new Frame(frameFlags, 0x1, &dataByte);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing data byte negative resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Shorten 6-th bit of data field by e TQ. This should be
                 *      a bit before stuff bit. Shorten both in driver and
                 *      monitor.
                 *   3. Force PH2 of 7-th bit of data field to Recessive. This
                 *      should be a stuff bit. Do it on driven frame only.
                 */
                monitor_bit_frame->TurnReceivedFrame();

                Bit *driverBeforeStuffBit = driver_bit_frame->GetBitOf(5, BitType::Data);
                Bit *monitorBeforeStuffBit = monitor_bit_frame->GetBitOf(5, BitType::Data);
                Bit *driverStuffBit = driver_bit_frame->GetBitOf(6, BitType::Data);

                driverBeforeStuffBit->ShortenPhase(BitPhase::Ph2, i);
                monitorBeforeStuffBit->ShortenPhase(BitPhase::Ph2, i);

                for (size_t j = 0; j < data_bit_timing.ph2_; j++)
                    driverStuffBit->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);

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
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};