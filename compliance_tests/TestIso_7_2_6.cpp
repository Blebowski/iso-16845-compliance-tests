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
 * @test ISO16845 7.2.6
 * 
 * @brief The purpose of this test is to verify that an IUT detecting a CRC
 *        error and a form error on the CRC delimiter in the same frame
 *        generates only one single 6 bits long error flag starting on the bit
 *        following the CRC delimiter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter, FDF = 0
 * 
 *  CAN FD Enabled
 *      CRC, DLC - to cause different CRC types. FDF = 1
 * 
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      #1 CRC (15)
 *
 *  CAN FD Enabled
 *      #1 DLC ≤ 10 − > CRC (17)
 *      #2 DLC > 10 − > CRC (21)
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with CRC error and form error at CRC delimiter
 *  according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate one active error frame starting at the bit position
 *  following the CRC delimiter.
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

class TestIso_7_2_6: public test_lib::TestBase
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
            int crcCnt;
            uint8_t dlcVals[3];
            FlexibleDataRate dataRate;

            // Generate random DLCs manually to meet constraints for CRCs
            dlcVals[0] = rand() % 9;
            if (rand() % 2 == 1)
                dlcVals[1] = 0x9;
            else
                dlcVals[1] = 0xA;
            dlcVals[2] = (rand() % 5) + 11;

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
                    crcCnt = 1;
                } else {
                    // Generate CAN frame (CAN FD, randomize others)
                    testMessage("CAN FD enabled part of test!");
                    dataRate = CAN_FD;
                    crcCnt = 3;
                }

                for (int j = 0; j < crcCnt; j++)
                {

                    FrameFlags frameFlags = FrameFlags(dataRate);
                    goldenFrame = new Frame(frameFlags, dlcVals[j]);
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
                     *   2. Force random CRC bit to its opposite value
                     *   3. Force CRC Delimiter to dominant.
                     *   4. Insert Error frame to position of ACK!
                     */
                    monitorBitFrame->turnReceivedFrame();

                    int crcIndex;
                    if (j == 0)
                        crcIndex = rand() % 15;
                    else if (j == 1)
                        crcIndex = rand() % 17;
                    else
                        crcIndex = rand() % 21;

                    testMessage("Forcing CRC bit nr: %d", crcIndex);
                    printf("Forcing CRC bit nr: %d\n", crcIndex);
                    driverBitFrame->getBitOfNoStuffBits(crcIndex, BitType::BIT_TYPE_CRC)->flipBitValue();

                    // TODO: Here we should re-stuff CRC because we might have added/removed
                    //       Stuff bit in CRC and causes length of model CRC and to be different!
                    driverBitFrame->getBitOf(0, BitType::BIT_TYPE_CRC_DELIMITER)->setBitValue(DOMINANT);

                    monitorBitFrame->insertActiveErrorFrame(
                        monitorBitFrame->getBitOf(0, BitType::BIT_TYPE_ACK));
                    driverBitFrame->insertActiveErrorFrame(
                        driverBitFrame->getBitOf(0, BitType::BIT_TYPE_ACK));

                    // Push frames to Lower tester, run and check!
                    pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                    runLowerTester(true, true);
                    checkLowerTesterResult();

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