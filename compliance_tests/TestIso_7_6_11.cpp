/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.11
 * 
 * @brief This test verifies that an error active IUT increases its REC by 8
 *        when detecting a dominant bit as the first bit after sending an error
 *        flag.
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
 *      #1 Dominant bit at the bit position following the end of the error flag
 *         sent by the IUT.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an active error flag in data field. The
 *  LT sends a dominant bit according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 after reception of the dominant
 *  bit sent by the LT.
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

class TestIso_7_6_11 : public test_lib::TestBase
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
            int rec;
            int recNew;
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

                // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
                // randomize Identifier 
                FrameFlags frameFlags = FrameFlags(dataRate, DATA_FRAME);
                goldenFrame = new Frame(frameFlags, 1, &dataByte);
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
                 *   2. Force 7-th bit of Data frame to opposite, this should be
                 *      stuff bit! This will cause stuff error!
                 *   3. Insert Active Error frame from 8-th bit of data frame!
                 *   4. Insert Dominant bit on first bit of Error delimiter. This
                 *      Will shift error delimiter by 1 bit since DUT shall wait
                 *      for monitoring Recessive bit!
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(6, BitType::BIT_TYPE_DATA)->flipBitValue();

                monitorBitFrame->insertActiveErrorFrame(
                    monitorBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));
                driverBitFrame->insertActiveErrorFrame(
                    driverBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));

                // Insert Dominant bit before first bit of Error delimiter!
                // On monitor, this bit shall be recessive!
                Bit *bit = driverBitFrame->getBitOf(0, BIT_TYPE_ERROR_DELIMITER);
                int bitIndex = driverBitFrame->getBitIndex(bit);

                driverBitFrame->insertBit(Bit(BIT_TYPE_ERROR_DELIMITER, DOMINANT,
                        &frameFlags, &nominalBitTiming, &dataBitTiming), bitIndex);
                monitorBitFrame->insertBit(Bit(BIT_TYPE_ERROR_DELIMITER, RECESSIVE,
                        &frameFlags, &nominalBitTiming, &dataBitTiming), bitIndex);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                // Check no frame is received by DUT
                if (dutIfc->hasRxFrame())
                {
                    testMessage("DUT has received frame but should not have!");
                    testResult = false;
                }

                // Check that REC has incremented by 9
                // (1 for original error frame, 8 for dominant bit after error flag)
                recNew = dutIfc->getRec();
                if (recNew != (rec + 9))
                {
                    testMessage("DUT REC not as expected. Expected %d, Real %d",
                                    rec + 9, recNew);
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