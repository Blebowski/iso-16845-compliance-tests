/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 2.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.1
 * 
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT shortens a dominant stuff bit in arbitration field by an amount of 
 *  Phase_Seg2(N) and then later shortens another dominant stuff bit by an
 *  amount of [Phase_Seg2(N) + 1] according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame on the bit position following the
 *  second shortened stuff bit.
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

class TestIso_7_7_1 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Enable TX to RX feedback
            CanAgentConfigureTxToRxFeedback(true);

            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/

            // CAN 2.0 frame, Base identifier, randomize others
            FrameFlags frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base);

            // Base ID full of 1s
            int id = pow(2,11) - 1;
            golden_frame = new Frame(frameFlags, 0x1, id);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("Sample point test.");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Shorten 6-th bit (1st stuff bit) of driven frame by PhaseSeg2.
             *   3. Shorten 12-th bit (2nd stuff bit) of driven frame still
             *      in Base ID by PhaseSeg2 + 1.
             *   4. Correct lenght of one of the monitored bits since second
             *      stuff bit causes negative re-synchronization.
             */
            monitor_bit_frame->TurnReceivedFrame();

            Bit *firstStuffBit = driver_bit_frame->GetStuffBit(0);
            firstStuffBit->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);

            Bit *secStuffBit = driver_bit_frame->GetStuffBit(1);
            secStuffBit->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            BitPhase prevPhase = secStuffBit->PrevBitPhase(BitPhase::Ph2);
            secStuffBit->ShortenPhase(prevPhase, 1);

            /* Compensate the monitored frame as if negative resynchronisation
               happend */
            size_t resync_amount = nominal_bit_timing.ph2_;
            if (nominal_bit_timing.sjw_ < resync_amount)
                resync_amount = nominal_bit_timing.sjw_;
            monitor_bit_frame->GetBitOf(11, BitType::BaseIdentifier)->ShortenPhase(
                BitPhase::Ph2, resync_amount);

            /* 5 + Stuff + 5 + Stuff = 12 bits. We need to insert error frame
               from 13-th bit on! */
            int bit_index = driver_bit_frame->GetBitIndex(
                            driver_bit_frame->GetBitOf(12, BitType::BaseIdentifier));

            /* Expected error frame on monitor (from start of bit after stuff bit)
             * Driver will have passive error frame -> transmitt all recessive */
            monitor_bit_frame->InsertActiveErrorFrame(bit_index);
            driver_bit_frame->InsertPassiveErrorFrame(bit_index);

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