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
 * @test ISO16845 8.2.7
 *
 * @brief This test verifies the behaviour in the CRC delimiter and acknowledge
 *        field when these fields are extended to 2 bits.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      CRC delimiter
 *      ACK slot
 *      FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 CRC delimiter up to 2-bit long (late ACK bit – long distance);
 *          #2 ACK up to 2-bit long (superposing ACK bits – near and long distance).
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT creates a CRC
 *  delimiter and an ACK bit as defined in elementary test cases.
 *
 * Response:
 *  The frame is valid. The IUT shall not generate an error frame.
 * 
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

class TestIso_8_2_7 : public test_lib::TestBase
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

            // For CAN FD enabled nodes only!
            if (canVersion != CAN_FD_ENABLED_VERSION)
                return false;

            /*****************************************************************
             * CRC Delimiter 2 bit long (i = 0), ACK bit 2 bit long (i = 1)
             ****************************************************************/
            for (int i = 0; i < 2; i++)
            {
                if (i == 0)
                    testMessage("Testing 2 bit long CRC delimiter!");
                else
                    testMessage("Testing 2 bit long ACK!");

                FrameFlags frameFlags = FrameFlags(CAN_FD, ESI_ERROR_ACTIVE);

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
                 *   1. Turn driven frame as received. Force ACK dominant.
                 *   2. For 2 bit CRC delimiter:
                 *          Insert one extra CRC delimiter bit to both driven
                 *          and monitored frame. This emulates late ACK.
                 *      For 2 bit ACK delimiter:
                 *          Insert one extra ACK bit to both driven and monitored
                 *          frame.
                 */
                driverBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                if (i == 0)
                {
                    Bit *crcDelimDriver = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                    Bit *crcDelimMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                    int crcDelimIndex = driverBitFrame->getBitIndex(crcDelimDriver);

                    // Copy CRC delimiter
                    // Note: All nominal bit timing will be used in copied bit!
                    driverBitFrame->insertBit(*crcDelimDriver, crcDelimIndex);
                    monitorBitFrame->insertBit(*crcDelimMonitor, crcDelimIndex);
                } else {
                    Bit *ackDriver = driverBitFrame->getBitOf(0, BIT_TYPE_ACK);
                    Bit *ackMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_ACK);
                    int ackIndex = driverBitFrame->getBitIndex(ackDriver);

                    // Copy ACK bit
                    driverBitFrame->insertBit(*ackDriver, ackIndex);
                    monitorBitFrame->insertBit(*ackMonitor, ackIndex);
                }

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