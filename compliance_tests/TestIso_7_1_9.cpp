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
 * @test ISO16845 7.1.9
 * 
 * @brief This test verifies the behaviour of the IUT when receiving two
 *        consecutive frames not separated by a bus idle state.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Intermission field length
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      Intermission field length
 *      FDF = 1
 * 
 * Elementary test cases:
 *      #1 The second frame starts after the second intermission bit of the
 *         first frame.
 *      #2 The second frame starts after the third intermission bit of the
 *         first frame.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  Two different test frames are used for each of the two elementary tests.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frames.
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

class TestIso_7_1_9 : public test_lib::TestBase
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
             * Common part of test
             ****************************************************************/
            testMessage("Common part of test!");

            for (int i = 0; i < 2; i++)
            {
                testBigMessage("\n\nIteration nr: %d\n", i + 1);

                // Generate 2 consecutivve frames
                // (Set CAN 2.0, randomize other)
                FrameFlags frameFlags = FrameFlags(CAN_2_0);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame 1:");
                goldenFrame->print();

                FrameFlags frameFlags2 = FrameFlags(CAN_2_0);
                goldenFrame2 = new Frame(frameFlags2);
                goldenFrame2->randomize();
                testBigMessage("Test frame 2:");
                goldenFrame2->print();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                driverBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame2 = new BitFrame(*goldenFrame2,
                    &this->nominalBitTiming, &this->dataBitTiming);

                // In first iteration, Intermission lasts only 2 bits ->
                // Remove last bit of intermission!
                if (i == 0)
                {
                    printf("Removing bit!\n");
                    driverBitFrame->removeBit(driverBitFrame->getBitOf(2, BitType::BIT_TYPE_INTERMISSION));
                    monitorBitFrame->removeBit(monitorBitFrame->getBitOf(2, BitType::BIT_TYPE_INTERMISSION));
                    printf("Removed bit!\n");
                }

                // Monitor frames as if received, driver frame must have ACK too!
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);
                monitorBitFrame2->turnReceivedFrame();
                driverBitFrame2->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                pushFramesToLowerTester(*driverBitFrame2, *monitorBitFrame2);
                runLowerTester(true, true);
                checkLowerTesterResult();

                // Read received frames from DUT and compare with sent frames
                Frame readFrame = this->dutIfc->readFrame();
                Frame readFrame2 = this->dutIfc->readFrame();
                if (compareFrames(*goldenFrame, readFrame) == false ||
                    compareFrames(*goldenFrame2, readFrame2) == false)
                {
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                }

                deleteCommonObjects();

                if (testResult == false)
                    return false;
            }

            /*****************************************************************
             * CAN FD Enabled part of test!
             ****************************************************************/
            if (canVersion == CAN_FD_ENABLED_VERSION)
            {
                testMessage("CAN FD Enabled part of test!");
                for (int i = 0; i < 2; i++)
                {
                    testBigMessage("\n\nIteration nr: %d\n", i + 1);

                    // Generate 2 consecutivve frames
                    // (Set CAN FD, randomize other)
                    FrameFlags frameFlags = FrameFlags(CAN_FD);
                    goldenFrame = new Frame(frameFlags);
                    goldenFrame->randomize();
                    testBigMessage("Test frame 1:");
                    goldenFrame->print();

                    FrameFlags frameFlags2 = FrameFlags(CAN_FD);
                    goldenFrame2 = new Frame(frameFlags2);
                    goldenFrame2->randomize();
                    testBigMessage("Test frame 2:");
                    goldenFrame2->print();

                    // Convert to Bit frames
                    driverBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    monitorBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    driverBitFrame2 = new BitFrame(*goldenFrame2,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    monitorBitFrame2 = new BitFrame(*goldenFrame2,
                        &this->nominalBitTiming, &this->dataBitTiming);

                    // In first iteration, Intermission lasts only 2 bits ->
                    // Remove last bit of intermission!
                    if (i == 0)
                    {
                        printf("Removing bit!\n");
                        driverBitFrame->removeBit(driverBitFrame->getBitOf(2, BitType::BIT_TYPE_INTERMISSION));
                        monitorBitFrame->removeBit(monitorBitFrame->getBitOf(2, BitType::BIT_TYPE_INTERMISSION));
                        printf("Removed bit!\n");
                    }

                    // Monitor frames as if received, driver frame must have ACK too!
                    monitorBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);
                    monitorBitFrame2->turnReceivedFrame();
                    driverBitFrame2->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

                    // Push frames to Lower tester, run and check!
                    pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                    pushFramesToLowerTester(*driverBitFrame2, *monitorBitFrame2);
                    runLowerTester(true, true);
                    checkLowerTesterResult();

                    // Read received frames from DUT and compare with sent frames
                    Frame readFrame = this->dutIfc->readFrame();
                    Frame readFrame2 = this->dutIfc->readFrame();
                    if (compareFrames(*goldenFrame, readFrame) == false ||
                        compareFrames(*goldenFrame2, readFrame2) == false)
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
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};