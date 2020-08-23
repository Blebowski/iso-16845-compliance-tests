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
 * @test ISO16845 7.8.8.1
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there is an additional recessive
 *        to dominant edge between two sample points where the first edge comes
 *        before the synchronization segment on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Bit start with negative offset and glitch between synchronization
 *      segment and sample point.
 *          ESI = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT reduce the length of BRS bit by one TQ(D) and the LT force
 *             the second TQ of ESI to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *  
 *  Additionally, the Phase_Seg2(D) of ESI bit shall be forced to recessive.
 *
 * Response:
 *  The modified ESI bit shall be sampled as dominant.
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

class TestIso_7_8_8_1 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift, ESI = 0
            FrameFlags frameFlags = FrameFlags(FrameType::CanFd, BrsFlag::Shift,
                                                EsiFlag::ErrorActive);
            golden_frame = new Frame(frameFlags);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("Glitch filtering test for negative phase error on ESI bit");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten BRS by 1 TQ in driven and monitored frame.
             *   3. Force 2nd TQ of ESI to Recessive.
             *   4. Force Phase 2 of ESI to Recessive.
             */
            monitor_bit_frame->TurnReceivedFrame();

            Bit *brsBit = driver_bit_frame->GetBitOf(0, BitType::Brs);
            Bit *brsBitMonitor = monitor_bit_frame->GetBitOf(0, BitType::Brs);
            Bit *esiBit = driver_bit_frame->GetBitOf(0, BitType::Esi);
            
            brsBit->ShortenPhase(BitPhase::Ph2, 1);
            brsBitMonitor->ShortenPhase(BitPhase::Ph2, 1);

            esiBit->ForceTimeQuanta(1, BitValue::Recessive);
            esiBit->ForceTimeQuanta(0, data_bit_timing.ph2_ - 1, BitPhase::Ph2,
                                    BitValue::Recessive);

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