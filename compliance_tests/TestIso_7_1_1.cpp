/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.1 (first part, CAN 2.0 frames)
 * 
 * @brief This test verifies the behaviour of the IUT when receiving a
 * correct data frame with different identifiers and different numbers of data
 * bytes in base format frame.
 * 
 * @version CAN FD Enabled, CAN FD Tolerant, Classical CAN
 * 
 * Test variables:
 *  ID
 *  DLC
 *  FDF = 0
 * 
 * Elementary test cases:
 *  The CAN ID will be element of: ∈ [000 h , 7FF h ]
 *  Different CAN IDs are used for test.
 *  Elementary test cases:
 *      #1 CAN ID = 555 h
 *      #2 CAN ID = 2AA h
 *      #3 CAN ID = 000 h
 *      #4 CAN ID = 7FF h
 *      #5 CAN ID = a random value
 *  Tested number of data bytes: ∈ [0, 8]
 *  Number of tests: 45
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The test system sends a frame with ID and DLC as specified in elementary
 *  test cases definition.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state should match the data
 *  sent in the test frame.
 * 
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

class TestIso_7_1_1 : public test_lib::TestBase
{
    public:

        // TODO: Use approach with randomization!
        FrameFlags frameFlags_2_0 = FrameFlags(CAN_2_0, BASE_IDENTIFIER,
            DATA_FRAME, BIT_RATE_DONT_SHIFT, ESI_ERROR_ACTIVE);
        FrameFlags frameFlags_fd = FrameFlags(CAN_FD, BASE_IDENTIFIER,
            DATA_FRAME, BIT_RATE_DONT_SHIFT, ESI_ERROR_ACTIVE);

        int idList[5];
        uint8_t data[64];

        /**
         * Test constructor.
         */
        TestIso_7_1_1() : TestBase()
        {
            idList[0] = 0x555;
            idList[1] = 0x2AA;
            idList[2] = 0x000;
            idList[3] = 0x7FF;
            idList[4] = rand() % (2 ^ 11);
            testResult = true;
        }

        /**
         * 
         */
        int run()
        {
            // Run Base test to setup TB
            TestBase::run();

            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * CAN 2_0, FD Tolerant, FD Enabled common part.
             ****************************************************************/

            for (int id = 0; id < 5; id++)
            {
                for (uint8_t dlc = 0; dlc <= 8; dlc++)
                {
                    // Generate random data
                    for (int k = 0; k < 64; k++)
                        data[k] = rand() % (2 ^ 8);

                    // Create frames
                    goldenFrame = new Frame(frameFlags_2_0, dlc, idList[id], data);
                    driverBitFrame = new BitFrame(*goldenFrame, &nominalBitTiming,
                                                    &dataBitTiming);
                    monitorBitFrame = new BitFrame(*goldenFrame, &nominalBitTiming,
                                                    &dataBitTiming);

                    testBigMessage("Test frame:");
                    goldenFrame->print();

                    // Monitor frame as if received, driver frame must have ACK too!
                    monitorBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

                    // Convert frames to test sequences, push to Lower tester, run and check!
                    pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                    runLowerTester(true, true);
                    checkLowerTesterResult();

                    // Read received frame from DUT and compare with sent frame
                    Frame readFrame = this->dutIfc->readFrame();
                    if (compareFrames(*goldenFrame, readFrame) == false)
                    {
                        testResult = false;
                        testControllerAgentEndTest(testResult);
                    }

                    deleteCommonObjects();

                    if (testResult == false)
                        return false;
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