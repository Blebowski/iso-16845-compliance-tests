/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 05.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.1
 *
 * @brief This test verifies the capacity of the IUT to transmit a frame with
 *        different identifiers and different numbers of data in a base format
 *        frame.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      ID
 *      DLC
 *      FDF = 0
 *
 * Elementary test cases:
 *      The CAN ID shall be element of:
 *          ∈ [000 h , 7FF h ]
 *      Different CAN IDs are used for test.
 *          #1 CAN ID = 555 h
 *          #2 CAN ID = 2AA h
 *          #3 CAN ID = 000 h
 *          #4 CAN ID = 7FF h
 *          #5 CAN ID = a random value
 *      Tested number of data bytes:∈ [0, 8].
 *      Number of tests: 9 × selected ID
 *
 * Setup:
 *  A single test frame is used for each elementary test. The LT causes the IUT
 *  to transmit a data frame with the parameters according to elementary test
 *  cases.
 *
 * Execution:
 *  The LT generates a frame with last CRC bit dominant.
 *  The LT forces the CRC delimiter bit to dominant according to elementary
 *  test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The content of the frame shall match the LT request.
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

class TestIso_8_1_1 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Start monitoring when DUT starts transmitting!
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);

            // Configure driver to wait for monitor so that LT sends ACK in right moment.
            CanAgentSetWaitForMonitor(true);

            // Enable TX/RX feedback so that DUT will see its own transmitted frame!
            CanAgentConfigureTxToRxFeedback(true);

            int identifiers[5] = {
                0x555,
                0x2AA,
                0x000,
                0x7FF,
                rand() % 0x7FF
            };

            for (uint8_t dlc = 0; dlc < 9; dlc++)
            {
                for (int id = 0; id < 5; id++)
                {
                    // CAN 2.0 Frame, Base ID only, Data frame
                    FrameFlags frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base,
                                                       RtrFlag::DataFrame);
                    golden_frame = new Frame(frameFlags, dlc, identifiers[id]);
                    golden_frame->Randomize();
                    TestBigMessage("Test frame:");
                    golden_frame->Print();

                    // Convert to Bit frames
                    driver_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitor_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    /**
                     * Modify test frames:
                     *   1. Turn driven frame as if received (insert ACK).
                     */
                    driver_bit_frame->TurnReceivedFrame();
                    
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