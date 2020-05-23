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
 * @test ISO16845 7.8.1.1
 * 
 * @brief The purpose of this test is to verify the position of the sample
 *        point of an IUT on bit position BRS.
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *   Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      BRS
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
 *      Test BRS #1:
 *          The LT forces the BRS bit to dominant from beginning up to one
 *          TQ(N) before Sampling_Point(N).
 *      Test BRS #2:
 *          The LT forces the BRS bit to dominant from beginning up to
 *          Sampling_Point(N).
 * 
 * Response:
 *  Test BRS #1:
 *      The modified BRS bit shall be sampled as recessive.
 *      The frame is valid. No error flag shall occur.
 *  Test BRS #2:
 *      The modified BRS bit shall be sampled as dominant.
 *      The frame is valid. No error flag shall occur. The bit rate will not
 *      switch for data phase.
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

class TestIso_7_8_1_1 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * BRS sampled Recessive (Shift) / BRS sample dominant (no shift)
             ****************************************************************/

            for (int i = 0; i < 2; i++)
            {
                // CAN 2.0 frame, Shift/ No shift based on iteration!
                FrameFlags frameFlags;
                if (i == 0)
                    frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
                else
                    frameFlags = FrameFlags(CAN_FD, BIT_RATE_DONT_SHIFT);

                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                if (i == 0)
                    testMessage("Testing BRS sampled Recessive");
                else
                    testMessage("Testing BRS sampled Dominant");

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Flip bit value to be sure that forced value has an
                 *      effect!
                 *   3. Force TSEG1 - 1 of BRS to dominant (i == 0), or TSEG1
                 *      of BRS to dominant (i == 1).
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                Bit *brsBit = driverBitFrame->getBitOf(0, BIT_TYPE_BRS);

                // For both set the orig. bit value to recessive so that we
                // see the dominant flipped bits!
                brsBit->setBitValue(RECESSIVE);

                int domPulseLength;
                
                if (i == 0)
                    domPulseLength = nominalBitTiming.prop + nominalBitTiming.ph1;
                else
                    domPulseLength = nominalBitTiming.prop + nominalBitTiming.ph1 + 1;

                for (int j = 0; j < domPulseLength; j++)
                    brsBit->forceTimeQuanta(j, DOMINANT);

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