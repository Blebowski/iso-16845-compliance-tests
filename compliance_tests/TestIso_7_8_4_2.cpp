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
 * @test ISO16845 7.8.4.2
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e > SJW(D) on bit position DATA.
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
 *              e ∈ {[SJW(D) + 1], [NTQ(D) − Phase_Seg2(D) − 1]{
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  Then, the recessive to dominant edge before this dominant stuff bit shall
 *  be delayed by additional e TQ(D)’s of recessive value at the beginning of
 *  this stuff bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed stuff bit to rece-
 *  ssive. This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after
 *  sampling point.
 *
 *  The LT forces a part of Phase_Seg2(D) of the delayed ESI bit to recessive.
 *  This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after sampling
 *  point.
 *
 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The wrong value of stuff bit shall cause an error flag.
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

class TestIso_7_8_4_2 : public test_lib::TestBase
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

            size_t upperTh = data_bit_timing.ph1_ + data_bit_timing.prop_ + 1;

            for (size_t i = data_bit_timing.sjw_ + 1; i < upperTh; i++)
            {
                // CAN FD frame with bit rate shift
                uint8_t dataByte = 0x7F;
                FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift);
                golden_frame = new Frame(frameFlags, 0x1, &dataByte);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing data byte positive resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Force first e time quantas of 7-th data bit to Recessive.
                 *      This bit should be dominant stuff bit.
                 *   3. Force 7-th data bit from SJW - 1 after sample point till the end to
                 *      Recessive.
                 *   4. Lengthen monitored 7-th data bit by SJW (this correspond to
                 *      DUTs resync. by SJW).
                 *   5. Insert active error frame from 8-th data bit further to monitored
                 *      frame. Insert passive error frame to driven frame!
                 */
                monitor_bit_frame->TurnReceivedFrame();

                Bit *driverStuffBit = driver_bit_frame->GetBitOf(6, BitType::Data);
                Bit *monitorStuffBit = monitor_bit_frame->GetBitOf(6, BitType::Data);

                // One bit after stuff bit will be recessive due to data byte. Insert
                // passive error frame from one bit further so that model does not modify
                // the stuff bit due to insertion of error frame after bit in data bit rate!
                Bit *driverNextBit = driver_bit_frame->GetBitOf(8, BitType::Data);
                Bit *monitorNextBit = monitor_bit_frame->GetBitOf(7, BitType::Data);

                for (size_t j = 0; j < i; j++)
                    driverStuffBit->ForceTimeQuanta(j, BitValue::Recessive);
                for (size_t j = data_bit_timing.sjw_ - 1; j < data_bit_timing.ph2_; j++)
                    driverStuffBit->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);

                monitorStuffBit->LengthenPhase(BitPhase::Sync, data_bit_timing.sjw_);

                driver_bit_frame->InsertPassiveErrorFrame(driverNextBit);
                monitor_bit_frame->InsertActiveErrorFrame(monitorNextBit);

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

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