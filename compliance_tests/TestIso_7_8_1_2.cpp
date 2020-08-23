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
 * @test ISO16845 7.8.1.2
 * 
 * @brief The purpose of this test is to verify the position of the sample
 *        point of an IUT on bit position DATA field.
 * 
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *      DATA field
 *      FDF = 1
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
 *  Test Data #1:
 *      The LT forces a recessive bit to dominant from beginning up to one TQ(D)
 *      before the sampling point.
 * 
 *  Test DATA #2:
 *      The LT forces a dominant bit to recessive for Phase_Seg2(D).
 * 
 * Response:
 *  Test DATA #1:
 *      The modified data bit shall be sampled as recessive.
 *      The frame is valid. DontShift error flag shall occur.
 * 
 *  Test DATA #2:
 *      The modified data bit shall be sampled as dominant.
 *      The frame is valid. DontShift error flag shall occur.
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

class TestIso_7_8_1_2 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // CAN FD enabled only!
            if (dut_can_version == CanVersion::Can_2_0 ||
                dut_can_version == CanVersion::CanFdTolerant)
            {
                test_result = false;
                return false;
            }

            // To avoid stuff bits in data field.
            uint8_t dataByteRecessiveSampled = 0x55;
            uint8_t dataByteDominantSampled = 0x15;

            /*****************************************************************
             * BRS sampled Recessive (Shift) / BRS sample dominant (no shift)
             ****************************************************************/

            for (int i = 0; i < 2; i++)
            {
                // CAN FD frame, shift bit rate
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift);

                // In 2nd iteration dominant bit will be sampled at second bit
                // position of data field! We must expect this in golden frame
                // so that it will be compared correctly with received frame!
                if (i == 0)
                    golden_frame = new Frame(frameFlags, 1, &dataByteRecessiveSampled);
                else
                    golden_frame = new Frame(frameFlags, 1, &dataByteDominantSampled);

                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                if (i == 0)
                    TestMessage("Testing Data bit sampled Recessive");
                else
                    TestMessage("Testing Data bit sampled Dominant");

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Modify 2nd bit of data field. Since data is 0x55
                 *      this bit is recessive. Flip its TSEG - 1 (i == 0)
                 *      or TSEG1 (i == 1) to dominant.
                 */
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                Bit *dataBit = driver_bit_frame->GetBitOf(1, BitType::Data);
                dataBit->bit_value_ = BitValue::Recessive;

                int domPulseLength;
                if (i == 0)
                    domPulseLength = data_bit_timing.prop_ + data_bit_timing.ph1_;
                else
                    domPulseLength = data_bit_timing.prop_ + data_bit_timing.ph1_ + 1;

                for (int j = 0; j < domPulseLength; j++)
                    dataBit->ForceTimeQuanta(j, BitValue::Dominant);    

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