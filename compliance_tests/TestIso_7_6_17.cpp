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
 * @test ISO16845 7.6.17
 * 
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when receiving a 13-bit length overload flag.
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
 *      #1 7 dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  a) The test system causes a receive error to initialize the REC value to 9.
 *  b) The LT causes the IUT to generate an overload frame after a valid frame
 *     reception (REC-1).
 *     After the overload flag sent by the IUT, the LT sends a sequence
 *     according to elementary test cases.
 *
 * Response:
 *  The correct frame up to the EOF will decrement REC and the overload
 *  enlargement will not increase REC.
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

class TestIso_7_6_17 : public test_lib::TestBase
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
                 *   4. Insert 7 Dominant bits to driver on can_tx and 7
                 *      Recessive bits to monitor on can_rx from first bit
                 *      of overload delimiter.
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);
                driverBitFrame->getBitOf(6, BIT_TYPE_EOF)->setBitValue(DOMINANT);

                monitorBitFrame->insertOverloadFrame(monitorBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));
                driverBitFrame->insertOverloadFrame(driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));

                Bit *overloadDelim = driverBitFrame->getBitOf(0, BIT_TYPE_OVERLOAD_DELIMITER);
                int bitIndex = driverBitFrame->getBitIndex(overloadDelim);

                for (int i = 0; i < 7; i++)
                {
                    driverBitFrame->insertBit(Bit(BIT_TYPE_OVERLOAD_FLAG, DOMINANT,
                        &frameFlags, &nominalBitTiming, &dataBitTiming), bitIndex);
                    monitorBitFrame->insertBit(Bit(BIT_TYPE_OVERLOAD_FLAG, RECESSIVE,
                        &frameFlags, &nominalBitTiming, &dataBitTiming), bitIndex);
                }

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

                // Check that REC was not incremented
                recNew = dutIfc->getRec();
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