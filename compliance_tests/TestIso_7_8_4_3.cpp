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
 * @test ISO16845 7.8.4.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e > SJW(D) on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          CRC: LSB = 1
 *          CRC delimiter
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              e ∈ {[SJW(D) + 1], [NTQ(D) − Phase_Seg2(D) − 1]{
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a test frame with a recessive bit value at last bit of CRC.
 *  The LT forces the CRC delimiter to dominant bit value.
 * 
 *  Then, the recessive to dominant edge between LSB of CRC and CRC delimiter
 *  shall be delayed by additional e TQ(D)’s of recessive value at the begi-
 *  nning of CRC delimiter bit according to elementary test cases.
 *  
 *  The LT forces a part of Phase_Seg2(D) of the delayed CRC delimiter bit to
 *  recessive. This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D)
 *  after sampling point.
 *
 * Response:
 *  The modified ESI bit shall be sampled as recessive.
 *  The frame is valid. DontShift error flag shall occur.
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

class TestIso_7_8_4_3 : public test_lib::TestBase
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

            int upperTh = data_bit_timing.ph1_ + data_bit_timing.prop_ + 1;

            for (int i = data_bit_timing.sjw_ + 1; i < upperTh; i++)
            {
                // CAN FD frame with bit rate shift, Base ID only and
                uint8_t dataByte = 0x55;
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                   RtrFlag::DataFrame, BrsFlag::Shift,
                                                   EsiFlag::ErrorActive);
                // Frame was empirically debugged to have last bit of CRC in 1!
                golden_frame = new Frame(frameFlags, 0x1, 50, &dataByte);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing CRC delimiter positive resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Force CRC delimiter to dominant value on driven frame.
                 *   3. Force first e TQs of CRC delimiter to Recessive.
                 *   4. Shorten PH2 of CRC Delimiter to 0 since this-one
                 *      is in multiples of nominal Time quanta. Lengthen
                 *      PH1 (still in data time quanta), by SJW - 1. This
                 *      has equal effect as forcing the bit to Recessive
                 *      SJW - 1 after sample point. Next bit is ACK which
                 *      is transmitted recessive by driver so this will act
                 *      as remaining recessive part of CRC delimiter!
                 */
                monitor_bit_frame->TurnReceivedFrame();

                Bit *crcDelimiter = driver_bit_frame->GetBitOf(0, BitType::CrcDelimiter);
                crcDelimiter->bit_value_ = BitValue::Dominant;

                for (int j = 0; j < i; j++)
                    crcDelimiter->ForceTimeQuanta(j, BitValue::Recessive);

                crcDelimiter->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
                BitPhase phase = crcDelimiter->PrevBitPhase(BitPhase::Ph2);
                crcDelimiter->LengthenPhase(phase, data_bit_timing.sjw_ - 1);

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