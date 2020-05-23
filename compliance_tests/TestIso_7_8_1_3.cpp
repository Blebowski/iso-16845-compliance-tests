/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 23.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.1.3
 * 
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *          CRC Delimiter
 *          FDF = 1
 * 
 * Elementary test cases:
 *      There are two elementary tests to perform for at least 1 bit rate
 *      configuration:
 *          #1 test for early sampling point: bit level change to recessive
 *             before sampling point;
 *          #2 test for late sampling point: bit level change to recessive
 *             after sampling point.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 * 
 *  Test CRC delimiter #1:
 *      The LT forces a recessive CRC delimiter bit to dominant from beginning
 *      up to one TQ(D) before the Sampling point.
 * 
 *  Test CRC delimiter #2:
 *      The LT forces a recessive CRC delimiter bit to dominant from beginning
 *      up to the sampling point.
 * 
 * Response:
 *  Test CRC delimiter #1:
 *      The modified CRC delimiter bit shall be sampled as recessive.
 *      The frame is valid. No error flag shall occur.
 * 
 *  Test CRC delimiter #2:
 *      The modified CRC delimiter bit shall be sampled as dominant.
 *      The frame is invalid. An error frame shall follow.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_7_8_1_3 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Enable TX to RX feedback
            canAgentConfigureTxToRxFeedback(true);

            // CAN FD enabled only!
            if (canVersion == CAN_2_0_VERSION ||
                canVersion == CAN_FD_TOLERANT_VERSION)
            {
                testResult = false;
                return false;
            }

            /*****************************************************************
             * CRC Delimiter sampled Recessive (OK) /
             * CRC Delimiter sampled Dominant (Error frame)
             ****************************************************************/
            for (int i = 0; i < 2; i++)
            {
                // CAN FD frame
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                if (i == 0)
                    testMessage("Testing CRC delimiter bit sampled Recessive");
                else
                    testMessage("Testing CRC delimiter bit sampled Dominant");

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Modify CRC Delimiter, flip TSEG1 - 1 (i == 0) or TSEG1
                 *      (i == 1) to dominant!
                 *   3. For i == 1, insert active error frame right after CRC
                 *      delimiter! Insert passive error frame to driver to send
                 *      all recessive (TX to RX feedback is turned ON)!
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *crcDelim = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                int bitIndex = driverBitFrame->getBitIndex(crcDelim);
                int domPulseLength;

                if (i == 0)
                    domPulseLength = dataBitTiming.prop + dataBitTiming.ph1;
                else
                    domPulseLength = dataBitTiming.prop + dataBitTiming.ph1 + 1;

                for (int j = 0; j < domPulseLength; j++)
                    crcDelim->forceTimeQuanta(j, DOMINANT);    

                if (i == 1)
                {
                    driverBitFrame->insertPassiveErrorFrame(bitIndex + 1);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex + 1);
                }

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                // Read received frame from DUT and compare with sent frame
                // (for i==0 only, i==1 ends with error frame)
                Frame readFrame = this->dutIfc->readFrame();
                if ((i == 0) && (compareFrames(*goldenFrame, readFrame) == false))
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