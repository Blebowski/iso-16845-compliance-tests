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
 * @test ISO16845 8.3.3
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a bit
 *        error when one of the 6 dominant bits of the error flag it transmits
 *        is forced to recessive state by LT.
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
 *          #1 corrupting the first bit of the error flag;
 *          #2 corrupting the fourth bit of the error flag;
 *          #3 corrupting the sixth bit of the error flag.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT corrupts this frame in data field causing the IUT to send an active
 *  error frame.
 *  Then the LT forces one of the 6 bits of the active error flag sent by the
 *  IUT to recessive state according to elementary test cases.
 * 
 * Response:
 *  The IUT shall restart its active error flag at the bit position following
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

class TestIso_8_3_3 : public test_lib::TestBase
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

            // Note: In this test we cant enable TX/RX feedback, since we want
            //       to corrupt DOMINANT active error flag! This is not possible
            //       when DUT transmitts dominant. We therefore disable it and
            //       compensate for what DUT is receiving, so that DUT will not
            //       see bit errors!

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
                    uint8_t dataByte = 0x80; // 7-th data bit will be recessive stuff bit
                    FrameFlags frameFlags;

                    if (i == 0)
                        frameFlags = FrameFlags(CAN_2_0, DATA_FRAME);
                    else
                        frameFlags = FrameFlags(CAN_FD, ESI_ERROR_ACTIVE);

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
                     *  1. Flip 7-th data bit of driven frame to dominant, this
                     *     will destroy recessive stuff bit send by IUT.
                     *  2. Insert expected active error frame from 8-th bit
                     *     of data field to monitored frame. Insert the same
                     *     to driven frame.
                     *  3. Flip 1,4 or 6-th bit of Error flag to Recessive. Insert
                     *     next expected error frame from one bit further.
                     *  4. Turn second driven frame (the same) as received. Append
                     *     after first frame. This checks retransmission.
                     * 
                     *  Note: TX/RX feedback is disabled, we need to drive the
                     *        same what we monitor so that IUT will see its own
                     *        frame!
                     */
                    driverBitFrame->getBitOf(6, BIT_TYPE_DATA)->setBitValue(DOMINANT);

                    int bitIndex = driverBitFrame->getBitIndex(
                        driverBitFrame->getBitOf(7, BIT_TYPE_DATA));
                    driverBitFrame->insertActiveErrorFrame(bitIndex);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex);

                    int bitIndexToFlip;
                    if (j == 0)
                        bitIndexToFlip = 1;
                    else if (j == 1)
                        bitIndexToFlip = 4;
                    else if (j == 2)
                        bitIndexToFlip = 6;

                    Bit *bitToFlip = driverBitFrame->getBitOf(bitIndexToFlip - 1, BIT_TYPE_ACTIVE_ERROR_FLAG);
                    bitToFlip->setBitValue(RECESSIVE);
                    int nextErrorFlagIndex = driverBitFrame->getBitIndex(bitToFlip) + 1;

                    driverBitFrame->insertActiveErrorFrame(driverBitFrame->getBit(nextErrorFlagIndex));
                    monitorBitFrame->insertActiveErrorFrame(monitorBitFrame->getBit(nextErrorFlagIndex));

                    // Append next frame. Needs to have ACK set!
                    secondDriverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);
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