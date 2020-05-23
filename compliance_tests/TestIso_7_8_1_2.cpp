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
 * @test ISO16845 7.8.1.2
 * 
 * @brief The purpose of this test is to verify the position of the sample
 *        point of an IUT on bit position DATA field.
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *      DATA field
 *      FDF = 1
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
 *  Test Data #1:
 *      The LT forces a recessive bit to dominant from beginning up to one TQ(D)
 *      before the sampling point.
 * 
 *  Test DATA #2:
 *      The LT forces a dominant bit to recessive for Phase_Seg2(D).
 * 
 * Response:
 *  Test DATA #1:
 *      The modified data bit shall be sampled as recessive.
 *      The frame is valid. No error flag shall occur.
 * 
 *  Test DATA #2:
 *      The modified data bit shall be sampled as dominant.
 *      The frame is valid. No error flag shall occur.
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

class TestIso_7_8_1_2 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // To avoid stuff bits in data field.
            uint8_t dataByteRecessiveSampled = 0x55;
            uint8_t dataByteDominantSampled = 0x15;

            /*****************************************************************
             * BRS sampled Recessive (Shift) / BRS sample dominant (no shift)
             ****************************************************************/

            for (int i = 0; i < 2; i++)
            {
                // CAN FD frame, shift bit rate
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);

                // In 2nd iteration dominant bit will be sampled at second bit
                // position of data field! We must expect this in golden frame
                // so that it will be compared correctly with received frame!
                if (i == 0)
                    goldenFrame = new Frame(frameFlags, 1, &dataByteRecessiveSampled);
                else
                    goldenFrame = new Frame(frameFlags, 1, &dataByteDominantSampled);

                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                if (i == 0)
                    testMessage("Testing Data bit sampled Recessive");
                else
                    testMessage("Testing Data bit sampled Dominant");

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Modify 2nd bit of data field. Since data is 0x55
                 *      this bit is recessive. Flip its TSEG - 1 (i == 0)
                 *      or TSEG1 (i == 1) to dominant.
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                Bit *dataBit = driverBitFrame->getBitOf(1, BIT_TYPE_DATA);
                dataBit->setBitValue(RECESSIVE);

                int domPulseLength;
                if (i == 0)
                    domPulseLength = dataBitTiming.prop + dataBitTiming.ph1;
                else
                    domPulseLength = dataBitTiming.prop + dataBitTiming.ph1 + 1;

                for (int j = 0; j < domPulseLength; j++)
                    dataBit->forceTimeQuanta(j, DOMINANT);    

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