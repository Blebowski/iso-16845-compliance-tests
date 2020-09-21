/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 10.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.5
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a negative phase error e on a recessive to dominant edge
 *        with |e| ≤ SJW(N).
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *      
 *      #1 The values tested for e are measured in time quanta with
 *          e ∈ [1, SJW(N)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT shortens the last recessive bit before an expected dominant stuff
 *  bit in arbitration field by an amount of |e| time quanta and then sends
 *  a dominant value for one time quantum followed by a recessive state
 *  according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame 1 bit time after the last recessive
 *  to dominant edge.
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

class TestIso_7_7_5 : public test_lib::TestBase
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

            for (size_t i = 0; i < nominal_bit_timing.sjw_; i++)
            {
                // CAN 2.0 frame, Base identifier, randomize others
                FrameFlags frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base);

                // Base ID full of 1s, 5th will be dominant stuff bit!
                int id = pow(2,11) - 1;
                golden_frame = new Frame(frameFlags, 0x1, id);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                TestMessage("Testing negative phase error: %d", i + 1);

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Shorten TSEG2 of bit before first stuff bit by e.
                 *      Shorten both in driven and monitored frame!
                 *   2. Set bit value of Dominant stuff bit to Recessive
                 *      apart from 1 TQ in the beginning of the bit for
                 *      driven frame!
                 *   3. Insert expected error frame one bit after first
                 *      stuff bit! Insert passive error frame on driver so
                 *      that it transmitts all recessive!
                 */
                monitor_bit_frame->TurnReceivedFrame();

                driver_bit_frame->GetBitOf(4, BitType::BaseIdentifier)
                    ->ShortenPhase(BitPhase::Ph2, i + 1);
                monitor_bit_frame->GetBitOf(4, BitType::BaseIdentifier)
                    ->ShortenPhase(BitPhase::Ph2, i + 1);

                Bit *stuffBit = driver_bit_frame->GetStuffBit(0);
                stuffBit->bit_value_ = BitValue::Recessive;
                stuffBit->GetTimeQuanta(0)->ForceValue(BitValue::Dominant);

                int index = driver_bit_frame->GetBitIndex(stuffBit);
                monitor_bit_frame->InsertActiveErrorFrame(index + 1);
                driver_bit_frame->InsertPassiveErrorFrame(index + 1);

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