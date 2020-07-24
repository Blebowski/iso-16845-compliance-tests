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
 * @test ISO16845 8.2.6
 *
 * @brief This test verifies that the IUT detects an acknowledgement error when
 *        the received ACK slot is recessive.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          ACK Slot = 1 bit, FDF = 0
 *      CAN FD enabled:
 *          ACK Slot = 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          #1 ACK slot = recessive
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a base format frame. Then, the LT does
 *  not send the ACK slot according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame starting at the bit position
 *  following the ACK slot.
 *  The IUT shall restart the transmission of the frame as soon as the bus is
 *  idle.
 * 
 * Note:
 *  For classical format frame usage, the IUT shall generate an error frame
 *  starting at the bit position following the 1-bit wide ACK slot.
 *  For FD format frame, the IUT shall generate an error frame starting at the
 *  bit position following the 2-bit wide ACK slot.
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

class TestIso_8_2_6 : public test_lib::TestBase
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
             * Common part of test (i=0), CAN FD enabled part of test(i=1)
             ****************************************************************/
            int iterCnt;
            FlexibleDataRate dataRate;

            if (canVersion == CAN_FD_ENABLED_VERSION)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                    testMessage("Common part of test!");
                else
                    testMessage("CAN FD enabled part of test!");

                FrameFlags frameFlags;

                if (i == 0)
                    frameFlags = FrameFlags(CAN_2_0);
                else
                    frameFlags = FrameFlags(CAN_FD, ESI_ERROR_ACTIVE);

                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                BitFrame *secDriverBitFrame = new BitFrame(*goldenFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);
                BitFrame *secMonitorBitFrame = new BitFrame(*goldenFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn driven frame as received, force ACK slot recessive
                 *      (to emulate missing ACK).
                 *   2. Insert expected error frame. In CAN 2.0, expected from
                 *      ACK Delimiter. In CAN FD. expect from EOF (as if ACK had
                 *      2 bits).
                 *   3. Append the same frame after the end of first one (to check
                 *      retransmission)
                 */
                driverBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(RECESSIVE);

                if (i == 0)
                {
                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(0, BIT_TYPE_ACK_DELIMITER));
                    driverBitFrame->insertPassiveErrorFrame(
                        driverBitFrame->getBitOf(0, BIT_TYPE_ACK_DELIMITER));
                } else {
                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(0, BIT_TYPE_EOF));
                    driverBitFrame->insertPassiveErrorFrame(
                        driverBitFrame->getBitOf(0, BIT_TYPE_EOF));
                }

                secDriverBitFrame->turnReceivedFrame();
                driverBitFrame->appendBitFrame(secDriverBitFrame);
                monitorBitFrame->appendBitFrame(secMonitorBitFrame);

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