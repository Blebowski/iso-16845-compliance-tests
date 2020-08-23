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
 * @test ISO16845 7.7.8
 * 
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there are two recessive to
 *        dominant edges between two sample points where the first edge comes
 *        before the synchronization segment. The test also verifies that an
 *        IUT is able to synchronize on a minimum duration pulse obeying to
 *        the synchronization rules.
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *      Glitch pulse length = 1 TQ(N)
 *          FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 Recessive glitch at third TQ(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in arbitration field.
 *  The recessive bit before the stuff bit is shortened by one time quantum.
 *  After the first two time quanta of dominant value, it changes one time
 *  quantum to recessive value according to elementary test cases. This dominant
 *  stuff bit is followed by 6 recessive bits.
 * 
 * Response:
 *  The IUT shall respond with an error frame exactly 7 bit times after the
 *  first recessive to dominant edge of the stuff bit.
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

class TestIso_7_7_8 : public test_lib::TestBase
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

            // Base ID full of 1s, 5th will be dominant stuff bit!
            int id = pow(2,11) - 1;
            golden_frame = new Frame(frameFlags, 0x1, id);
            golden_frame->Randomize();
            TestBigMessage("Test frame:");
            golden_frame->Print();

            TestMessage("Testing glitch filtering on negative phase error!");

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify test frames:
             *   1. Monitor frame as if received!
             *   2. Shorten bit before first stuff bit by 1 time quantum! Flip
             *      on both driven and monitored frame since DUT will re-sync.
             *      by 1!
             *   3. Flip third Time quanta of first stuff bit in arbitration
             *      field to recessive!
             *   4. ID contains all recessive. To reach sequence of 6 recessive
             *      bits, flip next stuff bit (2nd)
             *   5. Insert expected Error frame exactly after 6 bits after the
             *      end of first stuff bit (bit after 2nd stuff bit which had
             *      flipped value!). Insert Passive Error frame on driven frame
             *      so driver transmitts all recessive!
             */
            monitor_bit_frame->TurnReceivedFrame();

            driver_bit_frame->GetBitOf(4, BitType::BaseIdentifier)
                ->ShortenPhase(BitPhase::Ph2, 1);
            monitor_bit_frame->GetBitOf(4, BitType::BaseIdentifier)
                ->ShortenPhase(BitPhase::Ph2, 1);

            Bit *firstStuffBit = driver_bit_frame->GetStuffBit(0);
            firstStuffBit->GetTimeQuanta(2)->ForceValue(BitValue::Recessive);

            Bit *secondStuffBit = driver_bit_frame->GetStuffBit(1);
            secondStuffBit->bit_value_ = BitValue::Recessive;

            int index = driver_bit_frame->GetBitIndex(secondStuffBit);
            driver_bit_frame->InsertActiveErrorFrame(index + 1);
            monitor_bit_frame->InsertActiveErrorFrame(index + 1);

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