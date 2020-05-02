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
 * @test ISO16845 7.6.14
 * 
 * @brief This test verifies that the IUT decreases its REC by 1 when receiving
 *          a valid frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 * 
 *  CAN FD Enabled
 *      REC, FDF = 1
 * 
 * Elementary test cases:
 *  #1 One valid test frame.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame with a stuff error in it and force 1 bit of error
 *  flag to recessive.
 *  The LT sends a frame according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be decreased by 1 after the successful
 *  transmission of the ACK slot.
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

class TestIso_7_6_14 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * Setup part (to get REC to 9)
             ****************************************************************/
            testMessage("Setup part of test to get REC to 9!");

            // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
            // randomize Identifier 
            FrameFlags frameFlagsSetup = FrameFlags(CAN_2_0, DATA_FRAME);
            uint8_t dataByteSetup = 0x80;
            goldenFrame = new Frame(frameFlagsSetup, 1, &dataByteSetup);
            goldenFrame->randomize();
            testBigMessage("Setup frame:");
            goldenFrame->print();

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify setup frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
             *      This will cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Flip first bit of active error frame.
             *   5. Insert Error frame from first bit of Error frame further!
             */
            monitorBitFrame->turnReceivedFrame();
            driverBitFrame->getBitOf(6, BitType::BIT_TYPE_DATA)->flipBitValue();

            monitorBitFrame->insertActiveErrorFrame(
                monitorBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));
            driverBitFrame->insertActiveErrorFrame(
                driverBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));

            // Force 1st bit of Active Error flag on can_rx (driver) to RECESSIVE
            Bit *bit = driverBitFrame->getBitOf(0, BIT_TYPE_ACTIVE_ERROR_FLAG);
            bit->setBitValue(RECESSIVE);

            monitorBitFrame->insertActiveErrorFrame(
                monitorBitFrame->getBitOf(1, BIT_TYPE_ACTIVE_ERROR_FLAG));
            driverBitFrame->insertActiveErrorFrame(
                driverBitFrame->getBitOf(1, BIT_TYPE_ACTIVE_ERROR_FLAG));

            // Push frames to Lower tester, run and check!
            pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
            runLowerTester(true, true);
            checkLowerTesterResult();

            int recSetup = dutIfc->getRec();
            if (recSetup != 9)
            {
                testMessage("DUT REC not as expected. Expected %d, Real %d",
                                9, recSetup);
                testResult = false;
                testControllerAgentEndTest(testResult);
                return testResult;
            }
            deleteCommonObjects();

            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/

            int iterCnt;
            int rec;
            int recNew;
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

                // CAN 2.0 / CAN FD, randomize others
                FrameFlags frameFlags = FrameFlags(dataRate);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Read REC before scenario
                rec = dutIfc->getRec();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received.
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                recNew = dutIfc->getRec();

                // Check that REC was not incremented
                if (recNew != rec - 1)
                {
                    testMessage("DUT REC not as expected. Expected %d, Real %d",
                                    rec - 1, recNew);
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