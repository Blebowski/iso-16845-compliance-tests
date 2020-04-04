/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 3.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.4
 * 
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid base
 *        format frame.
 * 
 * @version CAN FD Enabled, Classical CAN
 * 
 * Test variables:
 *  Classical CAN  : FDF = 1
 *  CAN FD Enabled : FDF = 1, RRS = 1
 * 
 * Elementary test cases:
 *  Classical CAN:
 *      #1 FDF = 1
 * 
 *  CAN FD Enabled:
 *      #2 RRS = 1
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test cases.
 * 
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 * 
 * @todo: Classical CAN version not supported!
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

class TestIso_7_1_4 : public test_lib::TestBase
{
    public:
        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;

        TestIso_7_1_4() : TestBase(){};

        /*****************************************************************
         * Test sequence
         ****************************************************************/
        int run()
        {
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * Classical CAN part
             ****************************************************************/
            if (canVersion == CAN_2_0_VERSION)
            {
                testMessage("Classical CAN part of test not supporetd!");
                testControllerAgentEndTest(testResult);
                return false;
            }

            /*****************************************************************
             * CAN FD Enabled part
             ****************************************************************/
            if (canVersion == CAN_FD_ENABLED_VERSION)
            {
                testMessage("CAN FD ENABLED part of test");

                // Generate frame (Set Base ID, Data frame, randomize others)
                FrameFlags frameFlagsFd = FrameFlags(CAN_FD, BASE_IDENTIFIER,
                                                     DATA_FRAME);
                goldenFrame = new Frame(frameFlagsFd);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Convert to bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                // Force RRS bit to recessive, update frames (Stuff bits and CRC
                // might change)!
                driverBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);
                monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);

                // Update frames (Stuff bits, CRC might have changed!)
                driverBitFrame->updateFrame();
                monitorBitFrame->updateFrame();

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
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);

            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};