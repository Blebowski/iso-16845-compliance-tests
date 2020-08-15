/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 8.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.2
 *
 * @brief This test verifies that an IUT acting as a transmitter generates an
 *        overload frame when it detects a dominant bit on the eighth bit of
 *        an error or an overload delimiter.
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
 *      Elementary tests to perform:
 *          #1 dominant bit on the eighth bit of an error delimiter, error
 *             applied in data field;
 *          #2 dominant bit on the eighth bit of an overload delimiter
 *             following a data frame.
 * 
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an error frame or overload frame
 *  according to elementary test cases.
 *  Then, the LT forces the eighth bit of the delimiter to a dominant state.
 * 
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the dominant bit generated by the LT.
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

class TestIso_8_4_2 : public test_lib::TestBase
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

            // Set TEC, so that IUT becomes error passive. Keep sufficient
            // reserve from 128 for decrements due to test frames!
            dutIfc->setTec(200);

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

                    if (i == 0)
                        frameFlags = FrameFlags(CAN_2_0, DATA_FRAME);
                    else
                        frameFlags = FrameFlags(CAN_FD, ESI_ERROR_PASSIVE);

                    uint8_t dataByte = 0x80;
                    goldenFrame = new Frame(frameFlags, 0x1, &dataByte);
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
                     *  2. In first elementary test, force 7-th data bit
                     *     (should be recessive stuff bit) to dominant. In
                     *     second elementary test, force first bit of intermission
                     *     to dominant. Insert Error Frame (first elementary test)
                     *     or Overload frame (second elementary test) from next
                     *     bit on monitored frame. Insert passive Error frame
                     *     also to driven frame.
                     *  3. Force last bit of Error delimiter (first elementary test),
                     *     Overload delimiter (second elementary test) to dominant.
                     *  4. Insert Overload frame from next bit on monitored frame.
                     *     Insert Passive Error frame on driven frame so that LT does
                     *     not affect IUT.
                     *  5. Insert 8-more bits after intermission (behin 2-nd overload
                     *     frame). This emulates suspend transmission.
                     *  6. In first elemenary test, append the same frame after first
                     *     frame because frame shall be retransmitted (due to error
                     *     frame). This frame should immediately follow last bit of
                     *     suspend. In second elementary test, frame shall not be
                     *     re-transmitted, because there were only overload frames, so
                     *     append only dummy bits to check that unit does not retransmitt
                     *     (there were only overload frames)!
                     */
                    driverBitFrame->turnReceivedFrame();

                    if (j == 0)
                    {
                        Bit *dataStuffBit = driverBitFrame->getBitOf(6, BIT_TYPE_DATA);
                        dataStuffBit->setBitValue(DOMINANT);
                        monitorBitFrame->insertPassiveErrorFrame(monitorBitFrame->getBitOf(
                            7, BIT_TYPE_DATA));
                        driverBitFrame->insertPassiveErrorFrame(driverBitFrame->getBitOf(
                            7, BIT_TYPE_DATA));
                    }
                    else
                    {
                        Bit *firstIntermission = driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION);
                        firstIntermission->setBitValue(DOMINANT);
                        monitorBitFrame->insertOverloadFrame(monitorBitFrame->getBitOf(
                            1, BIT_TYPE_INTERMISSION));
                        driverBitFrame->insertPassiveErrorFrame(driverBitFrame->getBitOf(
                            1, BIT_TYPE_INTERMISSION));
                    }

                    Bit *lastDelimBit = driverBitFrame->getBitOf(7, BIT_TYPE_ERROR_DELIMITER);
                    int lastDelimIndex = driverBitFrame->getBitIndex(lastDelimBit);
                    lastDelimBit->setBitValue(DOMINANT);

                    monitorBitFrame->insertOverloadFrame(lastDelimIndex + 1);
                    driverBitFrame->insertPassiveErrorFrame(lastDelimIndex + 1);

                    // In second elementary test, last intermission bit is actually fourth
                    // intermission bit, because there is single bit of intermission before
                    // first error/overload fram!
                    int lastIntermissionIndex;
                    if (j == 0)
                        lastIntermissionIndex = 2;
                    else
                        lastIntermissionIndex = 3;

                    int endOfIntermissionIndex = driverBitFrame->getBitIndex(
                        driverBitFrame->getBitOf(lastIntermissionIndex, BIT_TYPE_INTERMISSION));
                    for (int k = 0; k < 8; k++)
                    {
                        driverBitFrame->insertBit(Bit(BIT_TYPE_SUSPEND, RECESSIVE,
                            &frameFlags, &nominalBitTiming, &dataBitTiming), endOfIntermissionIndex);
                        monitorBitFrame->insertBit(Bit(BIT_TYPE_SUSPEND, RECESSIVE,
                            &frameFlags, &nominalBitTiming, &dataBitTiming), endOfIntermissionIndex);
                    }

                    if (j == 0){
                        secondDriverBitFrame->turnReceivedFrame();
                        driverBitFrame->appendBitFrame(secondDriverBitFrame);
                        monitorBitFrame->appendBitFrame(secondMonitorBitFrame);
                    } else {
                        for (int k = 0; k < 15; k++)
                        {
                            driverBitFrame->insertBit(Bit(BIT_TYPE_IDLE, RECESSIVE,
                                &frameFlags, &nominalBitTiming, &dataBitTiming), endOfIntermissionIndex);
                            monitorBitFrame->insertBit(Bit(BIT_TYPE_IDLE, RECESSIVE,
                                &frameFlags, &nominalBitTiming, &dataBitTiming), endOfIntermissionIndex);
                        }
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