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

        FrameFlags frameFlags_2_0 = FrameFlags(CAN_2_0, BASE_IDENTIFIER,
            DATA_FRAME, BIT_RATE_DONT_SHIFT, ESI_ERROR_ACTIVE);
        FrameFlags frameFlags_fd = FrameFlags(CAN_FD, BASE_IDENTIFIER,
            DATA_FRAME, BIT_RATE_DONT_SHIFT, ESI_ERROR_ACTIVE);

        uint8_t data[64];
        int id;
        uint8_t dlc;

        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;
        test_lib::TestSequence *testSequence;

        /**
         * Test constructor.
         */
        TestIso_7_1_4() : TestBase()
        {
            id = rand() % (2 ^ 11);
            dlc = (rand() % 9);
            testResult = true;

            // Generate random data
            for (int k = 0; k < 64; k++)
                data[k] = rand() % (2 ^ 8);
        }

        /**
         * 
         */
        int run()
        {
            // Run Base test to setup TB
            TestBase::run();

            /*****************************************************************
             * Test sequence start
             ****************************************************************/

            testMessage("Test %s : Run Entered", testName);

            canAgentMonitorSetTrigger(CAN_AGENT_MONITOR_TRIGGER_DRIVER_START);

            /*****************************************************************
             * Classical CAN part
             ****************************************************************/

            if (canVersion == CAN_2_0_VERSION)
            {
                testMessage("Classical CAN part of test!");

                // Create frames
                goldenFrame = new Frame(frameFlags_2_0, dlc, id, data);
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                // TODO: Here protocol exception event should be turned ON!

                testMessage(std::string(80, '*'));
                testMessage("Test frame:");
                testMessage(std::string(80, '*'));
                goldenFrame->print();

                // Monitor frame as if received, driver frame must have ACK too!
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

                // Force r0/EDL bit to Recessive!
                driverBitFrame->getBit(14)->setBitValue(RECESSIVE);

                // Convert to test sequences and push to simulation
                testSequence = new test_lib::TestSequence(this->dutClockPeriod,
                    *driverBitFrame, *monitorBitFrame);
                testSequence->pushDriverValuesToSimulator();
                testSequence->pushMonitorValuesToSimulator();

                // Execute test
                canAgentMonitorStart();
                canAgentDriverStart();
                canAgentDriverWaitFinish();
                testMessage("Driver ended!");

                // Check and Cleanup
                canAgentCheckResult();
                canAgentMonitorStop();
                canAgentDriverStop();
                canAgentMonitorFlush();
                canAgentDriverFlush();

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
                delete testSequence;
            }

            /*****************************************************************
             * CAN FD Enabled part
             ****************************************************************/

            if (canVersion == CAN_FD_ENABLED_VERSION)
            {
                testMessage("Classical FD ENABLED part of test");

                // Create frames
                goldenFrame = new Frame(frameFlags_fd, dlc, id, data);
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                testMessage(std::string(80, '*'));
                testMessage("Test frame:");
                testMessage(std::string(80, '*'));
                goldenFrame->print();

                // Force RRS bit to recessive, update frames (Stuff bits and CRC)
                // might change!
                driverBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);
                monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);

                // Update frames (Stuff bits, CRC might have changed!)
                driverBitFrame->updateFrame();
                monitorBitFrame->updateFrame();

                // Monitor frame as if received, driver frame must have ACK too!
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

                // Convert to test sequences and push to simulation
                testSequence = new test_lib::TestSequence(this->dutClockPeriod,
                    *driverBitFrame, *monitorBitFrame);
                testSequence->pushDriverValuesToSimulator();
                testSequence->pushMonitorValuesToSimulator();

                // Execute test
                canAgentMonitorStart();
                canAgentDriverStart();
                canAgentDriverWaitFinish();
                testMessage("Driver ended!");

                // Check and Cleanup
                canAgentCheckResult();
                canAgentMonitorStop();
                canAgentDriverStop();
                canAgentMonitorFlush();
                canAgentDriverFlush();

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
                delete testSequence;
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};