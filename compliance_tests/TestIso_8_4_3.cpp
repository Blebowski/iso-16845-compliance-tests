/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 9.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.3
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a data frame starting with the identifier and without transmitting
 *        SOF, when detecting a dominant bit on the third bit of the intermi-
 *        ssion field following an overload frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 0
 *      CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 * 
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits.
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT disturbs the transmitted frame with an error frame, then the LT causes
 *  the IUT to generate an overload frame immediately after the error frame.
 *  Then, the LT forces the third bit of the intermission following the overload
 *  delimiter to dominant state.
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

class TestIso_8_4_3 : public test_lib::TestBase
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

                for (int j = 0; j < 2; j++)
                {
                    FrameFlags frameFlags;
                    uint8_t dataByte = 0x80;
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
                     *  2. Force 7-th data bit to Dominant. This should be recessive
                     *     stuff bit. Insert Active Error frame from next bit on, to
                     *     monitored frame. Insert Passive Error frame to driven frame.
                     *  3. Force 8-th bit of Error delimiter to dominant. Insert Overload
                     *     frame from next bit on to monitored frame. Insert Passive
                     *     Error frame to driven frame.
                     *  4. Force third bit of intermission after overload frame to
                     *     dominant (in driven frame).
                     *  5. Remove SOF bit from retransmitted frame. Append retransmitted
                     *     frame behind the first frame. Second driven frame is turned
                     *     received.
                     */
                    driverBitFrame->turnReceivedFrame();

                    Bit *dataStuffBit = driverBitFrame->getBitOf(6, BIT_TYPE_DATA);
                    dataStuffBit->setBitValue(DOMINANT);
                    monitorBitFrame->insertActiveErrorFrame(monitorBitFrame->getBitOf(
                        7, BIT_TYPE_DATA));
                    driverBitFrame->insertPassiveErrorFrame(driverBitFrame->getBitOf(
                        7, BIT_TYPE_DATA));

                    Bit *endOfErrorDelim = driverBitFrame->getBitOf(7, BIT_TYPE_ERROR_DELIMITER);
                    int endOfErrorDelimIndex = driverBitFrame->getBitIndex(endOfErrorDelim);
                    endOfErrorDelim->setBitValue(DOMINANT);
                    monitorBitFrame->insertOverloadFrame(endOfErrorDelimIndex + 1);
                    driverBitFrame->insertPassiveErrorFrame(endOfErrorDelimIndex + 1);

                    Bit *thirdIntermissionBit = driverBitFrame->getBitOf(2, BIT_TYPE_INTERMISSION);
                    thirdIntermissionBit->setBitValue(DOMINANT);

                    secondDriverBitFrame->turnReceivedFrame();
                    secondDriverBitFrame->removeBit(secondDriverBitFrame->getBitOf(0, BIT_TYPE_SOF));
                    secondMonitorBitFrame->removeBit(secondMonitorBitFrame->getBitOf(0, BIT_TYPE_SOF));

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