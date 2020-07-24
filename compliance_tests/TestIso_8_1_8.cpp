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
 * @test ISO16845 8.1.8
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a frame on reception of an SOF starting at the third bit of the
 *        intermission field following the arbitration winning frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          Intermission field 2 bits, FDF = 0
 *      CAN FD enabled:
 *          Intermission field 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits;
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT sends a frame with higher priority at the same time, to force an
 *  arbitration lost for the frame sent by the IUT. At start of intermission,
 *  the LT waits for 2 bit times before sending an SOF.
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

class TestIso_8_1_8 : public test_lib::TestBase
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

                for (int j = 0; j < 2; j++)
                {
                    FrameFlags frameFlags;

                    if (i == 0)
                        frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER);
                    else
                        frameFlags = FrameFlags(CAN_FD, BASE_IDENTIFIER);

                    int goldenIds[] = {
                        0x7B,
                        0x3B
                    };
                    int ltIds[] = {
                        0x7A,
                        0x3A
                    };

                    goldenFrame = new Frame(frameFlags, 0x0, goldenIds[j]);
                    Frame *ltFrame = new Frame(frameFlags, 0x0, ltIds[j]);

                    testBigMessage("Test frame:");
                    goldenFrame->print();

                    // Convert to Bit frames
                    driverBitFrame = new BitFrame(*ltFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    monitorBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);

                    BitFrame *secDriverBitFrame = new BitFrame(*goldenFrame,
                                                    &this->nominalBitTiming, &this->dataBitTiming);
                    BitFrame *secMonitorBitFrame = new BitFrame(*goldenFrame,
                                                    &this->nominalBitTiming, &this->dataBitTiming);

                    /**
                     * Modify test frames:
                     *   1. Loose arbitration on last bit of ID.
                     *   2. Force last bit of driven frame intermission to dominant.
                     *      This emulates LT sending SOF after 2 bits of intermission.
                     *   3. Append the same frame to driven and monitored frame.
                     *      On driven frame, turn second frame as if received.
                     *   4. Remove SOF from 2nd monitored frame.
                     */
                    monitorBitFrame->looseArbitration(
                        monitorBitFrame->getBitOfNoStuffBits(10, BIT_TYPE_BASE_ID));

                    driverBitFrame->getBitOf(2, BIT_TYPE_INTERMISSION)->setBitValue(DOMINANT);
                    
                    secDriverBitFrame->turnReceivedFrame();

                    secMonitorBitFrame->removeBit(secMonitorBitFrame->getBitOf(0, BIT_TYPE_SOF));
                    secDriverBitFrame->removeBit(secDriverBitFrame->getBitOf(0, BIT_TYPE_SOF));

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
                    
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};