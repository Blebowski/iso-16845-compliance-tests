/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 19.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.5
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a
 *        form error when it receives an invalid overload delimiter.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          FDF = 0
 *      CAN FD Enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 * 
 *      Elementary tests to perform:
 *          #1 corrupting the second bit of the overload delimiter.
 *          #2 corrupting the fourth bit of the overload delimiter.
 *          #3 corrupting the seventh bit of the overload delimiter.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame.
 *  The LT corrupts the overload delimiter according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame starting at the bit position after
 *  the corrupted bit.
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

class TestIso_8_4_5 : public test_lib::TestBase
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

            canAgentConfigureTxToRxFeedback(true);

            int iterCnt;

            if (canVersion == CAN_FD_ENABLED_VERSION)
                iterCnt = 2;
            else
                iterCnt = 1;

            /*****************************************************************
             * Common part of test (i=0), CAN FD enabled part of test(i=1)
             ****************************************************************/
            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                    testMessage("CAN 2.0 part of test");
                else
                    testMessage("CAN FD part of test");

                for (int j = 0; j < 3; j++)
                {
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

                    BitFrame *secondDriverBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    BitFrame *secondMonitorBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);

                    /**
                     * Modify test frames:
                     *  1. Turn driven frame as received.
                     *  2. Force first bit of Intermission to Dominant.
                     *     (Overload condition)
                     *  3. Insert Overload frame from second bit of Intermission to
                     *     monitored frame.
                     *  4. Force 2, 4, 7-th bit of Overload delimiter to Dominant.
                     *  5. Insert Passive Error frame from next bit to driven frame.
                     *     Insert Active Error frame to monitored frame.
                     *
                     *  Note: Don't insert retransmitted frame after first frame, since
                     *        error happened in overload frame which was transmitted
                     *        due to Overload condition in Intermission. At this point
                     *        frame has already been validated by transmitter! This is
                     *        valid according to spec. since for transmitter frame
                     *        vaidation shall occur at the end of EOF!
                     */
                    driverBitFrame->turnReceivedFrame();

                    Bit *firstIntermissionBit = driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION);
                    firstIntermissionBit->setBitValue(DOMINANT);

                    driverBitFrame->insertOverloadFrame(
                        driverBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION));
                    monitorBitFrame->insertOverloadFrame(
                        monitorBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION));

                    Bit *bitToCorrupt;
                    if (j == 0){
                        bitToCorrupt = driverBitFrame->getBitOf(1, BIT_TYPE_OVERLOAD_DELIMITER);
                    } else if (j == 1){
                        bitToCorrupt = driverBitFrame->getBitOf(3, BIT_TYPE_OVERLOAD_DELIMITER);
                    } else {
                        bitToCorrupt = driverBitFrame->getBitOf(6, BIT_TYPE_OVERLOAD_DELIMITER);
                    }

                    int bitIndex = driverBitFrame->getBitIndex(bitToCorrupt);
                    bitToCorrupt->setBitValue(DOMINANT);

                    driverBitFrame->insertPassiveErrorFrame(bitIndex + 1);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex + 1);

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
                    delete secondDriverBitFrame;
                    delete secondMonitorBitFrame;
                }
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};