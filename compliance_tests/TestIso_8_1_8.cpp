/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 24.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.8
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a frame on reception of an SOF starting at the third bit of the
 *        intermission field following the arbitration winning frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          Intermission field 2 bits, FDF = 0
 *      CAN FD enabled:
 *          Intermission field 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits;
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT sends a frame with higher priority at the same time, to force an
 *  arbitration lost for the frame sent by the IUT. At start of intermission,
 *  the LT waits for 2 bit times before sending an SOF.
 *
 * Response:
 *  The IUT shall repeat the frame starting with the identifier without
 *  transmitting any SOF.
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

class TestIso_8_1_8 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Start monitoring when DUT starts transmitting!
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));

            // Configure driver to wait for monitor so that LT sends ACK in right moment.
            CanAgentSetWaitForMonitor(true);

            // Enable TX/RX feedback so that DUT will see its own transmitted frame!
            CanAgentConfigureTxToRxFeedback(true);

            /*****************************************************************
             * Common part of test (i=0), CAN FD enabled part of test(i=1)
             ****************************************************************/
            int iterCnt;

            if (dut_can_version == CanVersion::CanFdEnabled)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                    TestMessage("Common part of test!");
                else
                    TestMessage("CAN FD enabled part of test!");

                for (int j = 0; j < 2; j++)
                {
                    FrameFlags frameFlags;

                    if (i == 0)
                        frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base);
                    else
                        frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base);

                    int goldenIds[] = {
                        0x7B,
                        0x3B
                    };
                    int ltIds[] = {
                        0x7A,
                        0x3A
                    };

                    golden_frame = new Frame(frameFlags, 0x0, goldenIds[j]);
                    Frame *ltFrame = new Frame(frameFlags, 0x0, ltIds[j]);

                    TestBigMessage("Test frame:");
                    golden_frame->Print();

                    // Convert to Bit frames
                    driver_bit_frame = new BitFrame(*ltFrame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitor_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    BitFrame *secDriverBitFrame = new BitFrame(*golden_frame,
                                                    &this->nominal_bit_timing, &this->data_bit_timing);
                    BitFrame *secMonitorBitFrame = new BitFrame(*golden_frame,
                                                    &this->nominal_bit_timing, &this->data_bit_timing);

                    /**
                     * Modify test frames:
                     *   1. Loose arbitration on last bit of ID.
                     *   2. Force last bit of driven frame intermission to dominant.
                     *      This emulates LT sending SOF after 2 bits of intermission.
                     *   3. Append the same frame to driven and monitored frame.
                     *      On driven frame, turn second frame as if received.
                     *   4. Remove SOF from 2nd monitored frame.
                     */
                    monitor_bit_frame->LooseArbitration(
                        monitor_bit_frame->GetBitOfNoStuffBits(10, BitType::BaseIdentifier));

                    driver_bit_frame->GetBitOf(2, BitType::Intermission)
                        ->bit_value_ = BitValue::Dominant;
                    
                    secDriverBitFrame->TurnReceivedFrame();

                    secMonitorBitFrame->RemoveBit(secMonitorBitFrame->GetBitOf(0, BitType::Sof));
                    secDriverBitFrame->RemoveBit(secDriverBitFrame->GetBitOf(0, BitType::Sof));

                    driver_bit_frame->AppendBitFrame(secDriverBitFrame);
                    monitor_bit_frame->AppendBitFrame(secMonitorBitFrame);

                    driver_bit_frame->Print(true);
                    monitor_bit_frame->Print(true);

                    // Push frames to Lower tester, insert to DUT, run and check!
                    PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                    StartDriverAndMonitor();

                    TestMessage("Sending frame via DUT!");
                    this->dut_ifc->SendFrame(golden_frame);
                    TestMessage("Sent frame via DUT!");
                    
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();

                    DeleteCommonObjects();
                }
                    
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};