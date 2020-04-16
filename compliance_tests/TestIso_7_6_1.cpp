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
 * @test ISO16845 7.3.3
 * 
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        a bit error during the transmission of an active error flag.
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
 *      #1 corrupting the first bit of the error flag;
 *      #2 corrupting the third bit of the error flag;
 *      #3 corrupting the sixth bit of the error flag.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field.
 *  The LT forces one of the bits of the error frame generated by the IUT to
 *  recessive state according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 on the corrupted bit.
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

class TestIso_7_6_1 : public test_lib::TestBase
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

                for (int j = 0; j < 3; j++)
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
                        bitToCorrupt = 1;
                    else if (j == 1)
                        bitToCorrupt = 3;
                    else
                        bitToCorrupt = 6;

                    testMessage("Forcing Error flag bit %d to recessive", bitToCorrupt);

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
                     *   4. Flip 1st, 3rd or 6th bit of Active Error flag to RECESSIVE.
                     *   5. Insert next Error frame one bit after form error in Error flag!
                     */
                    monitorBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(6, BitType::BIT_TYPE_DATA)->flipBitValue();

                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));
                    driverBitFrame->insertActiveErrorFrame(
                        driverBitFrame->getBitOf(7, BitType::BIT_TYPE_DATA));

                    // Force n-th bit of Active Error flag on can_rx (driver) to RECESSIVE
                    Bit *bit = driverBitFrame->getBitOf(bitToCorrupt - 1, BIT_TYPE_ACTIVE_ERROR_FLAG);
                    int bitIndex = driverBitFrame->getBitIndex(bit);
                    bit->setBitValue(RECESSIVE);

                    // Insert new error flag from one bit further, both driver and monitor!
                    driverBitFrame->insertActiveErrorFrame(bitIndex + 1);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex + 1);

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
                    // (1 for original error frame, 8 for next error frame)
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
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};