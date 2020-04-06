/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 6.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.12
 * 
 * @brief The purpose of this test is to verify the point of time at which a
 *        message is taken to be valid by the IUT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      EOF, FDF = 0
 * 
 *  CAN FD Enabled
 *      EOF, FDF = 1
 * 
 * Elementary test cases:
 *      #1 The last bit of the EOF is forced to dominant state.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The IUT shall generate an overload frame.
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

class TestIso_7_1_12 : public test_lib::TestBase
{
    public:

        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;

        TestIso_7_1_12() : TestBase()
        {}

        /*****************************************************************
         * Test sequence
         ****************************************************************/
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

            if (canVersion == CAN_FD_ENABLED_VERSION)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                {
                    // Generate CAN frame (CAN 2.0, randomize others)
                    testMessage("Common part of test!");
                    dataRate = CAN_2_0;
                } else {
                    // Generate CAN frame (CAN FD, randomize others)
                    testMessage("CAN FD enabled part of test!");
                    dataRate = CAN_FD;
                }
                FrameFlags frameFlags = FrameFlags(dataRate);
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
                 *   1. Force last bit of EOF of driven frame to 0 (Overload condition)
                 *   2. Put ACK to driven frame too!
                 *   2. Turn monitored frame to received frame!
                 *   3. Insert overload frame to monitored/driven frame on first bit of
                 *      intermission.
                 */
                driverBitFrame->getBitOf(6, BitType::BIT_TYPE_EOF)->setBitValue(DOMINANT);
                driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);
                monitorBitFrame->turnReceivedFrame();
                monitorBitFrame->insertOverloadFrame(
                    monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_INTERMISSION));
                driverBitFrame->insertOverloadFrame(
                    driverBitFrame->getBitOf(0, BitType::BIT_TYPE_INTERMISSION));

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                // Read received frames from DUT and compare with sent frames
                Frame readFrame = this->dutIfc->readFrame();
                if (compareFrames(*goldenFrame, readFrame) == false)
                {
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                }

                delete goldenFrame;
                delete driverBitFrame;
                delete monitorBitFrame;
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};