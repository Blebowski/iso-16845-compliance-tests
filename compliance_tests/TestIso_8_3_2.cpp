/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.3.2
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a frame on reception of an SOF starting at the third bit of the
 *        intermission field following the error frame it has transmitted.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          Intermission field = 2 bit,
 *          FDF = 0
 *      CAN FD Enabled:
 *          Intermission field = 2 bit,
 *          FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits.
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test cases.
 *  The LT corrupts this frame in data field causing the IUT to send an active
 *  error frame. At the end of the error flag sent by the IUT, the LT waits for
 *  (8 + 2) bit times before sending SOF.
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

class TestIso_8_3_2 : public test_lib::TestBase
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

                for (int j = 0; j < 2; j++)
                {
                    uint8_t dataByte = 0x80; // 7-th data bit will be recessive stuff bit
                    FrameFlags frameFlags;
                    int ids[] = {
                        0x7B,
                        0x3B
                    };

                    if (i == 0)
                        frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER, DATA_FRAME);
                    else
                        frameFlags = FrameFlags(CAN_FD, BASE_IDENTIFIER, ESI_ERROR_ACTIVE);

                    goldenFrame = new Frame(frameFlags, 0x1, ids[j], &dataByte);
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
                     *  2. Flip 7-th data bit of driven frame to dominant, this
                     *     will destroy recessive stuff bit send by IUT.
                     *  3. Insert expected active error frame from 8-th bit
                     *     of data field to monitored frame. Insert the same
                     *     to driven frame.
                     *  4. Flip last bit of Intermission to dominant. This emulates
                     *     SOF sent to DUT.
                     *  5. Turn second driven frame (the same) as received. Remove
                     *     SOF in both driven and monitored frames. Append after
                     *     first frame. This checks retransmission.
                     */
                    driverBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(6, BIT_TYPE_DATA)->setBitValue(DOMINANT);

                    int bitIndex = driverBitFrame->getBitIndex(
                        driverBitFrame->getBitOf(7, BIT_TYPE_DATA));
                    driverBitFrame->insertActiveErrorFrame(bitIndex);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex);

                    driverBitFrame->getBitOf(2, BIT_TYPE_INTERMISSION)->setBitValue(DOMINANT);

                    secondDriverBitFrame->turnReceivedFrame();
                    secondDriverBitFrame->removeBit(
                        secondDriverBitFrame->getBitOf(0, BIT_TYPE_SOF));
                    secondMonitorBitFrame->removeBit(
                        secondMonitorBitFrame->getBitOf(0, BIT_TYPE_SOF));
                    driverBitFrame->appendBitFrame(secondDriverBitFrame);
                    monitorBitFrame->appendBitFrame(secondMonitorBitFrame);

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
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};