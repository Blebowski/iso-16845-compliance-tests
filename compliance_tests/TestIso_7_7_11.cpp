/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.3
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a negative phase error e on a recessive to dominant edge
 *        with |e| ≤ SJW(N) on bit position ACK.
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *   Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             |e| ∈ [1, SJW(N)].
 *      
 *      Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame.
 *  The LT forces an amount of e TQ from end of CRC delimiter bit to dominant.
 *  Additionally, the ACK bit shall be forced to recessive from end of bit
 *  toward Sampling_Point(N) for Phase_Seg2(N) + e according to elementary
 *  test cases. The bit shall be sampled as dominant.
 * 
 * Response:
 *  The frame is valid, no error flag shall occur.
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

class TestIso_7_7_11 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/

            for (int i = 0; i < nominalBitTiming.sjw; i++)
            {
                // CAN 2.0 frame, randomize others
                FrameFlags frameFlags = FrameFlags(CAN_2_0);

                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing ACK negative phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Shorten PH2 phase of CRC Delimiter by e. Shorten in
                 *      both driven and monitored frame since DUT shall re-sync!
                 *   3. Force PH2 of ACK to recessive.
                 * 
                 * Note: This is not exactly sequence as described in ISO,
                 *       there bits are not shortened but flipped, but overall
                 *       effect is the same!
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *crcDelimiter = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                crcDelimiter->shortenPhase(PH2_PHASE, i + 1);
                crcDelimiter = monitorBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                crcDelimiter->shortenPhase(PH2_PHASE, i + 1);

                Bit *ack = driverBitFrame->getBitOf(0, BIT_TYPE_ACK);
                ack->setBitValue(DOMINANT);

                ack->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);

                // Shorten monitored ACK by 1 TQ since DUT will re-synchronise
                // with SYNC segment completed!
                Bit *ackMon = monitorBitFrame->getBitOf(0, BIT_TYPE_ACK);
                ackMon->shortenPhase(PH1_PHASE, 1);

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