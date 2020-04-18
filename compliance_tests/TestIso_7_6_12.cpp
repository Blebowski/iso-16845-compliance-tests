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
 * @test ISO16845 7.6.12
 * 
 * @brief This test verifies that a receiver increases its REC by 1 when
 *        detecting a form error on a bit of the error delimiter it is
 *        transmitting.
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
 *      #1 the second bit of the error delimiter is corrupted;
 *      #2 the seventh bit of the error delimiter is corrupted.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  The LT corrupts 1 bit of the error delimiter according to elementary
 *  test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 after reception of the
 *  dominant bit sent by the LT.
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

class TestIso_7_6_12 : public test_lib::TestBase
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
            uint8_t dataByte = 0x80;
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

                for (int j = 0; j < 2; j++)
                {
                    // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
                    // randomize Identifier 
                    FrameFlags frameFlags = FrameFlags(dataRate, DATA_FRAME);
                    goldenFrame = new Frame(frameFlags, 1, &dataByte);
                    goldenFrame->randomize();
                    testBigMessage("Test frame:");
                    goldenFrame->print();

                    // Read REC before scenario
                    rec = dutIfc->getRec();

                    int bitToCorrupt;
                    if (j == 0)
                        bitToCorrupt = 2;
                    else
                        bitToCorrupt = 7;

                    testMessage("Forcing Error delimiter bit %d to Dominant",
                                    bitToCorrupt);

                    // Convert to Bit frames
                    driverBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);
                    monitorBitFrame = new BitFrame(*goldenFrame,
                        &this->nominalBitTiming, &this->dataBitTiming);

                    /**
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
                     *      This will cause stuff error!
                     *   3. Insert Active Error frame from 8-th bit of data frame!
                     *   4. Flip 2nd or 7th bit of Error delimiter to dominant!
                     *   5. Insert next active error frame from 3-rd or 8-th bit
                     *      of Error delimiter!
                     */
                    monitorBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(6, BitType::BIT_TYPE_DATA)->flipBitValue();

                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));
                    driverBitFrame->insertActiveErrorFrame(
                        driverBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));

                    // Force n-th bit of Error Delimiter to dominant!
                    Bit *bit = driverBitFrame->getBitOf(bitToCorrupt - 1, BIT_TYPE_ERROR_DELIMITER);
                    int bitIndex = driverBitFrame->getBitIndex(bit);
                    bit->setBitValue(DOMINANT);

                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(bitToCorrupt,BIT_TYPE_ERROR_DELIMITER));
                    driverBitFrame->insertActiveErrorFrame(
                        driverBitFrame->getBitOf(bitToCorrupt, BIT_TYPE_ERROR_DELIMITER));

                    driverBitFrame->print(true);
                    monitorBitFrame->print(true);

                    // Push frames to Lower tester, run and check!
                    pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                    runLowerTester(true, true);
                    checkLowerTesterResult();

                    recNew = dutIfc->getRec();

                    // Check that REC was incremented by 2
                    // (1 stuff error in data field, 1 form error in error delimiter!)
                    if (recNew != rec + 2)
                    {
                        testMessage("DUT REC not as expected. Expected %d, Real %d",
                                        rec + 2, recNew);
                        testResult = false;
                        testControllerAgentEndTest(testResult);
                        return testResult;
                    }
                    deleteCommonObjects();
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