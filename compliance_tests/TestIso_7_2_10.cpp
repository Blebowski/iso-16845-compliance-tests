/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 11.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.10
 * 
 * @brief This test verifies that the IUT detects a form error when one of 6
 *        first recessive bits of EOF is forced to dominant state by LT.
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
 *      #1 to #5 corrupting the first until the fifth bit position.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 * 
 * Response:
 *  The IUT shall generate an active error frame at the bit position following
 *  the corrupted bit.
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

class TestIso_7_2_10 : public test_lib::TestBase
{
    public:

        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;

        TestIso_7_2_10() : TestBase()
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
                    testMessage("Common part of test!");
                    dataRate = CAN_2_0;
                } else {
                    testMessage("CAN FD enabled part of test!");
                    dataRate = CAN_FD;
                }

                for (int j = 0; j < 5; j++)
                {
                    // CAN 2.0 / CAN FD , randomize others
                    FrameFlags frameFlags = FrameFlags(dataRate);
                    goldenFrame = new Frame(frameFlags);
                    goldenFrame->randomize();
                    testBigMessage("Test frame:");
                    goldenFrame->print();
                    testMessage("Corrupting EOF bit: %d", j + 1);

                    // Convert to Bit frames
                    driverBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    monitorBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);

                    /**
                     * Modify test frames:
                     *   1. Monitor frame as if received, insert ACK.
                     *   2. j-th bit of EOF forced to dominant!
                     *   3. Insert Active Error frame from first bit of EOF!
                     */
                    monitorBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(0, BitType::BIT_TYPE_ACK)->setBitValue(DOMINANT);
                    driverBitFrame->getBitOf(j, BitType::BIT_TYPE_EOF)->setBitValue(DOMINANT);

                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(j + 1, BitType::BIT_TYPE_EOF));
                    driverBitFrame->insertActiveErrorFrame(
                        driverBitFrame->getBitOf(j + 1, BitType::BIT_TYPE_EOF));

                    driverBitFrame->print(true);
                    monitorBitFrame->print(true);

                    // Push frames to Lower tester, run and check!
                    pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                    runLowerTester(true, true);
                    checkLowerTesterResult();

                    // Check no frame is received by DUT
                    if (dutIfc->hasRxFrame())
                        testResult = false;

                    delete goldenFrame;
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