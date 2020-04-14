/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.3.2
 * 
 * @brief The purpose of this test is to verify that an IUT accepts a frame
 *        starting after the second bit of the intermission following the error
 *        frame it has transmitted.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      FDF = 1
 * 
 * Elementary test cases:
 *      #1 Frame is started 2 bits after the end of the error delimiter.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field.
 *  The LT sends a valid frame according to elementary test cases.
 * 
 * Response:
 *  The IUT shall acknowledge the test frame in data field.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

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

class TestIso_7_3_2 : public test_lib::TestBase
{
    public:

        Frame *goldenFrame2;
        BitFrame *driverBitFrame2;
        BitFrame *monitorBitFrame2;

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/

            int iterCnt;
            FlexibleDataRate dataRate;
            uint8_t dataByte = 0x80;

            if (canVersion == CAN_FD_ENABLED_VERSION)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                {
                    testMessage("Common part of test!");
                    dataRate = CAN_2_0;
                } else {
                    testMessage("CAN FD enabled part of test!");
                    dataRate = CAN_FD;
                }

                // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
                // randomize Identifier 
                FrameFlags frameFlags = FrameFlags(dataRate, DATA_FRAME);
                goldenFrame = new Frame(frameFlags, 1, &dataByte);
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
                 *   1. Monitor frame as if received.
                 *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
                 *      This will cause stuff error!
                 *   3. Insert Active Error frame from 8-th bit of data frame!
                 *   4. Remove last bit of Intermission (after error frame)
                 *   5. Insert second frame directly after first frame.
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(6, BitType::BIT_TYPE_DATA)->flipBitValue();

                monitorBitFrame->insertActiveErrorFrame(
                    monitorBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));
                driverBitFrame->insertActiveErrorFrame(
                    driverBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));

                driverBitFrame->removeBit(driverBitFrame->getBitOf(2, BIT_TYPE_INTERMISSION));
                monitorBitFrame->removeBit(monitorBitFrame->getBitOf(2, BIT_TYPE_INTERMISSION));

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Generate frame 2 - randomize everything
                FrameFlags frameFlags2 = FrameFlags();
                goldenFrame2 = new Frame(frameFlags);
                goldenFrame2->randomize();
                testBigMessage("Test frame 2:");
                goldenFrame2->print();

                driverBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame2->turnReceivedFrame();
                driverBitFrame2->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                pushFramesToLowerTester(*driverBitFrame2, *monitorBitFrame2);
                runLowerTester(true, true);
                checkLowerTesterResult();

                // Read out frame from DUT and check it!
                Frame readFrame = this->dutIfc->readFrame();
                if (compareFrames(*goldenFrame2, readFrame) == false)
                {
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                }

                deleteCommonObjects();

                delete goldenFrame2;
                delete driverBitFrame2;
                delete monitorBitFrame2;
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};