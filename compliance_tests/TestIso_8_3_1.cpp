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
 * @test ISO16845 8.3.1
 *
 * @brief This test verifies that an IUT acting as a transmitter tolerates up
 *        to 7 dominant bits after sending its own error flag.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *          FDF = 0
 *      CAN FD Enabled
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 the LT extends the error flag by 1 dominant bit;
 *          #2 the LT extends the error flag by 4 dominant bits;
 *          #3 the LT extends the error flag by 7 dominant bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. The LT corrupts this frame in
 *  data field causing the IUT to send an active error frame. The LT prolongs
 *  the error flag sent by IUT according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate only one error frame.
 *  The IUT shall restart the transmission after the intermission field
 *  following the error frame.
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

class TestIso_8_3_1 : public test_lib::TestBase
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
                     *  1. Turn driven frame as received.
                     *  2. Flip 7-th data bit of driven frame to dominant, this
                     *     will destroy recessive stuff bit send by IUT.
                     *  3. Insert expected active error frame from 8-th bit
                     *     of data field to monitored frame. Insert the same
                     *     to driven frame.
                     *  4. Insert 1,4,7 dominant bits to driven frame after active
                     *     error flag in driven frame (prolong error flag). Insert
                     *     equal amount of recessive bits to monitored frame (this
                     *     corresponds to accepting longer Error flag without
                     *     re-sending next error flag).
                     *  5. Append the same frame second time. This checks
                     *     retransmission.
                     */
                    driverBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(6, BIT_TYPE_DATA)->setBitValue(DOMINANT);

                    int bitIndex = driverBitFrame->getBitIndex(
                        driverBitFrame->getBitOf(7, BIT_TYPE_DATA));
                    driverBitFrame->insertActiveErrorFrame(bitIndex);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex);

                    int bitsToInsert;
                    if (j == 0)
                        bitsToInsert = 1;
                    else if (j == 1)
                        bitsToInsert = 4;
                    else
                        bitsToInsert = 7;

                    Bit *firstErrDelimBit = driverBitFrame->getBitOf(0, BIT_TYPE_ERROR_DELIMITER);
                    int firstErrDelimIndex = driverBitFrame->getBitIndex(firstErrDelimBit);

                    for (int k = 0; k < bitsToInsert; k++)
                    {
                        driverBitFrame->insertBit(Bit(BIT_TYPE_ACTIVE_ERROR_FLAG, DOMINANT,
                            &frameFlags, &nominalBitTiming, &dataBitTiming), firstErrDelimIndex);
                        monitorBitFrame->insertBit(Bit(BIT_TYPE_PASSIVE_ERROR_FLAG, RECESSIVE,
                            &frameFlags, &nominalBitTiming, &dataBitTiming), firstErrDelimIndex);
                    }

                    secondDriverBitFrame->turnReceivedFrame();
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