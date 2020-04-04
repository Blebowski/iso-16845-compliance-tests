/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.8
 * 
 * @brief This test verifies the behaviour of the IUT when receiving a correct
 *        classical frame with a DLC greater than 8.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  DLC, FDF = 0
 * 
 * Elementary test cases:
 *  There are seven elementary tests, for which DLC ∈ [9h , Fh].
 * 
 *      TEST    DLC
 *       #1     0x9
 *       #2     0xA
 *       #3     0xB
 *       #4     0xC
 *       #5     0xD
 *       #6     0xE
 *       #7     0xF
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test cases.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data and DLC received by the IUT during the test state shall match the
 *  data and DLC sent in the test frame.
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

class TestIso_7_1_8 : public test_lib::TestBase
{
    public:

        uint8_t dlcs[7] = {0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;

        TestIso_7_1_8() : TestBase()
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
             * Common part of test
             ****************************************************************/
            testMessage("Common part of test!");

            for (int i = 0; i < 7; i++)
            {
                testBigMessage("\n\nIteration nr: %d\n", i + 1);

                // Generate frame (Set DLC and CAN 2.0, randomize other)
                FrameFlags frameFlagsFd = FrameFlags(CAN_2_0);
                goldenFrame = new Frame(frameFlagsFd, dlcs[i]);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                // Monitor frame as if received, driver frame must have ACK too!
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

                // Push frames to Lower tester, run and check!
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

                delete goldenFrame;
                delete driverBitFrame;
                delete monitorBitFrame;

                if (testResult == false)
                    return false;
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};