/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 12.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.5
 *
 * @brief The purpose of this test is to verify the point of time at which a
 *        message transmitted by the IUT is taken to be valid.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled - FDF = 0
 *      CAN FD enabled - FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 * 
 *      #1 On the first bit of the intermission field of the frame sent by the
 *         IUT, the LT forces the bit value to dominant.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a data frame. The LT causes the IUT to
 *  generate an overload frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall not restart any frame after the overload frame.
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

class TestIso_8_1_5 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Start monitoring when DUT starts transmitting!
            canAgentMonitorSetTrigger(CAN_AGENT_MONITOR_TRIGGER_TX_FALLING);
            canAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));

            // Configure driver to wait for monitor so that LT sends ACK in right moment.
            canAgentSetWaitForMonitor(true);

            // Enable TX/RX feedback so that DUT will see its own transmitted frame!
            canAgentConfigureTxToRxFeedback(true);


            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/
            for (int i = 0; i < 2; i++)
            {
                // CAN 2.0 Frame, Base ID only, Data frame
                FrameFlags frameFlags;
                
                if (i == 0){
                    frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER, DATA_FRAME);
                } else {
                    frameFlags = FrameFlags(CAN_FD, ESI_ERROR_ACTIVE);
                }

                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn driven frame as if received (insert ACK).
                 *   2. Force first bit of driven frame to dominant.
                 *   3. Insert overload flag from 2nd bit of intermission further!
                 *   4. Insert 15 recessive bits at the end of Overload delimiter.
                 *      This checks that DUT does not retransmitt the frame!
                 */
                driverBitFrame->turnReceivedFrame();

                driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION)->setBitValue(DOMINANT);

                Bit *overloadStartBit = monitorBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION);
                monitorBitFrame->insertOverloadFrame(overloadStartBit);

                Bit *overloadEndBit = monitorBitFrame->getBitOf(6, BIT_TYPE_OVERLOAD_DELIMITER);
                int bitIndex = monitorBitFrame->getBitIndex(overloadEndBit);
                for (int i = 0; i < 15; i++)
                    monitorBitFrame->insertBit(Bit(BIT_TYPE_OVERLOAD_DELIMITER, RECESSIVE,
                                                   &frameFlags, &nominalBitTiming, &dataBitTiming),
                                                bitIndex);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, insert to DUT, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                startDriverAndMonitor();

                testMessage("Sending frame via DUT!");
                this->dutIfc->sendFrame(goldenFrame);
                testMessage("Sent frame via DUT!");
                
                waitForDriverAndMonitor();
                checkLowerTesterResult();

                deleteCommonObjects();
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};