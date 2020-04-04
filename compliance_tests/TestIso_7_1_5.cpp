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
 * @test ISO16845 7.1.5
 * 
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid
 *        extended format frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN   : SRR, FDF, r0
 *  CAN FD Tolerant : SRR, FDF, r0=0
 *  CAN FD Enabled  : SRR, RRS, FDF=1
 * 
 * Elementary test cases:
 *  Classical CAN:
 *      TEST    SRR     r0      FDF
 *       #1      1       1       1
 *       #2      1       1       0
 *       #3      1       0       1
 *       #4      0       1       1
 *       #5      0       1       0
 *       #6      0       0       1
 *       #7      0       0       0   
 * 
 *  CAN FD Tolerant:
 *      TEST    SRR     r0
 *       #1      1       1
 *       #2      0       1
 *       #3      0       0
 *
 *  CAN FD Enabled:
 *      TEST    SRR     RRS
 *       #1      1       1
 *       #2      0       1
 *       #3      0       0
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
 * @todo Only CAN FD Enabled so far implemented!
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

class TestIso_7_1_5 : public test_lib::TestBase
{
    public:

        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;

        TestIso_7_1_5() : TestBase()
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
             * Classical CAN part / CAN FD Tolerant version
             ****************************************************************/
            if (canVersion == CAN_2_0_VERSION ||
                canVersion == CAN_FD_TOLERANT_VERSION)
            {
                testMessage("Unsupported CAN version type!");
                testControllerAgentEndTest(false);
                return false;
            }

            /*****************************************************************
             * CAN FD Enabled part
             ****************************************************************/
            if (canVersion == CAN_FD_ENABLED_VERSION)
            {
                testMessage("CAN FD enabled part of test!");

                for (int i = 1; i <= 3; i++)
                {
                    // Generate frame (Set Extended ID, Data frame, randomize others)
                    FrameFlags frameFlagsFd = FrameFlags(CAN_FD, EXTENDED_IDENTIFIER,
                                                         DATA_FRAME);
                    goldenFrame = new Frame(frameFlagsFd);
                    goldenFrame->randomize();
                    testBigMessage("Test frame:");
                    goldenFrame->print();
                    testBigMessage("Iteration nr: %d", i);

                    // Convert to bit frames
                    driverBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    monitorBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);

                    // Force bits like so:
                    //  TEST    SRR     RRS
                    //   #1      1       1
                    //   #2      0       1
                    //   #3      0       0
                    switch (i)
                    {
                    case 1:
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_SRR)->setBitValue(RECESSIVE);
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_SRR)->setBitValue(RECESSIVE);
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);
                        break;
                    case 2:
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_SRR)->setBitValue(DOMINANT);
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_SRR)->setBitValue(DOMINANT);
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(RECESSIVE);
                        break;
                    case 3:
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_SRR)->setBitValue(DOMINANT);
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(DOMINANT);
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_SRR)->setBitValue(DOMINANT);
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_R1)->setBitValue(DOMINANT);
                        break;
                    }

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

                    delete driverBitFrame;
                    delete monitorBitFrame;
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