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
 * @test ISO16845 7.8.9.1
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        BRS.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          BRS = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the first two TQ(N) and the complete Phase_Seg2(N)
 *             of BRS bit to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant BRS bit.
 *  The LT inverts parts of BRS bit according to elementary test cases.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur. The bit rate will not
 *  switch for data phase.
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

class TestIso_7_8_9_1 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift, Dont shift bit rate
            // Here we have to set Bit rate dont shift because we intend to
            // get BRS dominant, so bit rate should not be shifted!
            FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::DontShift);
            golden_frame = new Frame(frameFlags);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("No synchronisation after dominant bit sampled on BRS bit!");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Flip BRS value to dominant.
             *   3. Force first two TQ of BRS and Phase 2 of BRS to
             *      Recessive. This should cause resynchronisation edge
             *      with phase error 2, but DUT shall ignore it and not
             *      resynchronize because previous bit (r0) was Dominant!
             */
            monitor_bit_frame->TurnReceivedFrame();

            Bit *brsBit = driver_bit_frame->GetBitOf(0, BitType::Brs);
            
            brsBit->bit_value_ = BitValue::Dominant;
            
            brsBit->ForceTimeQuanta(0, BitValue::Recessive);
            brsBit->ForceTimeQuanta(1, BitValue::Recessive);

            // Force all TQ of PH2 as if no shift occured (this is what frame
            // was generated with)
            brsBit->ForceTimeQuanta(0, nominal_bit_timing.ph2_ - 1,
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