/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.16
 * 
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when detecting a dominant bit at the last bit of the EOF it is
 *        receiving.
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
 *      #1 Last bit of the EOF.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The test system causes a receive error to initialize the REC value to 9.
 *  The LT sends one valid test frame according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be 8.
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

class TestIso_7_6_16 : public test_lib::TestBase
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
            int rec, recNew;
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

                dutIfc->setRec(9);
                rec = dutIfc->getRec();
                if (rec != 9)
                {
                    testBigMessage("REC not set properly to 9!");
                    testResult = false;
                }

                // CAN 2.0 / CAN FD, randomize others
                FrameFlags frameFlags = FrameFlags(dataRate);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Forcing last bit of EOF to dominant!");

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received, insert ACK to driven frame.
                 *   2. Force last bit of EOF to DOMINANT.
                 *   3. Insert expected overload frame from first bit of Intermission.
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);
                driverBitFrame->getBitOf(6, BIT_TYPE_EOF)->setBitValue(DOMINANT);

                monitorBitFrame->insertOverloadFrame(monitorBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));
                driverBitFrame->insertOverloadFrame(driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));

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

                // Check that REC has decremented!
                recNew = dutIfc->getRec();

                // Check that REC was not incremented
                if (recNew != 8)
                {
                    testMessage("DUT REC not as expected. Expected %d, Real %d",
                                    8, recNew);
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                    return testResult;
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