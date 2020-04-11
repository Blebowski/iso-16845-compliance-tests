/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 6.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.9
 * 
 * @brief This test verifies that the IUT detects a form error when the
 *        recessive ACK delimiter is forced to dominant state by LT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ACK delimiter, ACK = 0, FDF = 0
 * 
 *  CAN FD Enabled
 *      ACK delimiter, ACK1 = 0, ACK2 = 0, FDF = 1
 * 
 * Elementary test cases:
 *      #1 ACK delimiter = 0
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with form error at ACK delimiter according to
 *  elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an active error frame at the bit position following
 *  the ACK delimiter.
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

class TestIso_7_2_9 : public test_lib::TestBase
{
    public:

        Frame *goldenFrame;
        BitFrame *driverBitFrame;
        BitFrame *monitorBitFrame;

        TestIso_7_2_9() : TestBase()
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
                    // Generate CAN frame (CAN 2.0, randomize others)
                    testMessage("Common part of test!");
                    dataRate = CAN_2_0;
                } else {
                    // Generate CAN frame (CAN FD, randomize others)
                    testMessage("CAN FD enabled part of test!");
                    dataRate = CAN_FD;
                }
                FrameFlags frameFlags = FrameFlags(dataRate);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
            
                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received.
                 *   2. CAN 2.0 -> Force ACK Delimiter DOMINANT
                 *      CAN FD -> Force first bit of ACK to dominant
                 *             -> Add second ACK bit, driven dominant, monitored recessive
                 *   3. Insert Active Error frame from first bit of EOF!
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BitType::BIT_TYPE_ACK)->setBitValue(DOMINANT);
                driverBitFrame->getBitOf(0, BitType::BIT_TYPE_ACK_DELIMITER)->setBitValue(DOMINANT);

                if (dataRate == CAN_FD)
                {
                    Bit *ackDelimBit = driverBitFrame->getBitOf(0, BitType::BIT_TYPE_ACK_DELIMITER);
                    int ackDelimIndex = driverBitFrame->getBitIndex(ackDelimBit);
                    driverBitFrame->insertBit(
                        Bit(BitType::BIT_TYPE_ACK, DOMINANT, &frameFlags,
                        &this->nominalBitTiming, &this->dataBitTiming, STUFF_NO), ackDelimIndex);
                    monitorBitFrame->insertBit(
                        Bit(BitType::BIT_TYPE_ACK, RECESSIVE, &frameFlags,
                        &this->nominalBitTiming, &this->dataBitTiming, STUFF_NO), ackDelimIndex);
                }

                monitorBitFrame->insertActiveErrorFrame(
                    monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_EOF));
                driverBitFrame->insertActiveErrorFrame(
                    driverBitFrame->getBitOf(0, BitType::BIT_TYPE_EOF));

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

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