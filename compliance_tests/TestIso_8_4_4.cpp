/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 15.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.4
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a bit
 *        error when one of the 6 dominant bits of the overload flag it
 *        transmits is forced to recessive state by LT.
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
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 * 
 *      Elementary tests to perform:
 *          #1 corrupting the first bit of the overload flag;
 *          #2 corrupting the second bit of the overload flag;
 *          #3 corrupting the sixth bit of the overload flag.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame.
 *  Then, the LT corrupts one of the 6 dominant bits of the overload flag to
 *  the recessive state according to elementary test cases.
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

class TestIso_8_4_4 : public test_lib::TestBase
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

            /* Dont enable TX to RX feedback beacuse we need to force Dominant
             * overload flag to be received as Recessive!
             */

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
                     *  1. Force ACK low in driven frame (TX/RX feedback not enabled!)
                     *  2. Force first bit of Intermission to Dominant.
                     *     (Overload condition)
                     *  3. Insert Overload frame from second bit of Intermission to
                     *     monitored frame.
                     *  4. Force 1, 2, 6-th bit of Overload flag to Recessive.
                     *  5. Insert Active Error frame from next bit to driven frame.
                     *     Insert Passive Error frame to monitored frame.
                     *
                     *  Note: Don't insert retransmitted frame after first frame, since
                     *        error happened in overload frame which was transmitted
                     *        due to Overload condition in Intermission. At this point
                     *        frame has already been validated by transmitter! This is
                     *        valid according to spec. since for transmitter frame
                     *        vaidation shall occur at the end of EOF!
                     */
                    driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                    Bit *firstIntermissionBit = driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION);
                    firstIntermissionBit->setBitValue(DOMINANT);

                    driverBitFrame->insertOverloadFrame(
                        driverBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION));
                    monitorBitFrame->insertOverloadFrame(
                        monitorBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION));

                    Bit *bitToCorrupt;
                    if (j == 0){
                        bitToCorrupt = driverBitFrame->getBitOf(0, BIT_TYPE_OVERLOAD_FLAG);
                    } else if (j == 1){
                        bitToCorrupt = driverBitFrame->getBitOf(1, BIT_TYPE_OVERLOAD_FLAG);
                    } else {
                        bitToCorrupt = driverBitFrame->getBitOf(5, BIT_TYPE_OVERLOAD_FLAG);
                    }

                    int bitIndex = driverBitFrame->getBitIndex(bitToCorrupt);
                    bitToCorrupt->setBitValue(RECESSIVE);

                    driverBitFrame->insertActiveErrorFrame(bitIndex + 1);
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