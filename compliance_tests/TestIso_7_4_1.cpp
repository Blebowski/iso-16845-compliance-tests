/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 13.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.4.1
 * 
 * @brief This test verifies that the IUT generates an overload frame when
 *        detecting a dominant bit on one of the 2 first recessive bits of the
 *        intermission field.
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
 *      #1 first bit of intermission;
 *      #2 second bit of intermission.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  One test frame is used for each of the two elementary tests.
 *  The LT forces one of the 2 first bits of the intermission field of the test
 *  frame to dominant state according to elementary test cases.
 * 
 * Response:
 *  The IUT generates an overload frame at the bit position following the
 *  dominant bit.
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

class TestIso_7_4_1 : public test_lib::TestBase
{
    public:

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

                // CAN 2.0 / CAN FD, randomize others
                FrameFlags frameFlags = FrameFlags(dataRate);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Forcing bits 1 and 2 of Intermission to dominant");

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received, insert ACK to driven frame.
                 *   2. Force 1st bit of Intermission to DOMINANT.
                 *   3. Insert expected overload frame.
                 *   4. Force 2nd bit of Intermission to DOMINANT.
                 *   5. Insert expected overload frame.
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);
                driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION)->setBitValue(DOMINANT);

                monitorBitFrame->insertOverloadFrame(monitorBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION));
                driverBitFrame->insertOverloadFrame(driverBitFrame->getBitOf(1, BIT_TYPE_INTERMISSION));
                
                // Now bit on index 2 is actually 2nd bit of intermission after first
                // overload frame (because there remained 1 bit in first intermission)!
                driverBitFrame->getBitOf(2, BIT_TYPE_INTERMISSION)->setBitValue(DOMINANT);
                
                monitorBitFrame->insertOverloadFrame(monitorBitFrame->getBitOf(3, BIT_TYPE_INTERMISSION));
                driverBitFrame->insertOverloadFrame(driverBitFrame->getBitOf(3, BIT_TYPE_INTERMISSION));
                
                driverBitFrame->print(true);
                monitorBitFrame->print(true);

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

                deleteCommonObjects();
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};