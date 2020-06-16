/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 16.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW on bit position ACK.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *          Phase error e
 *          ACK
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              |e| ∈ [1, SJW(N)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame.
 *  The LT shortened the CRC delimiter by an amount of |e| TQ according to
 *  elementary test cases.
 *  Additionally, the Phase_Seg2(N) of this dominant ACK bit shall be forced
 *  to recessive.
 *
 * Response:
 *  The modified ACK bit shall be sampled as dominant.
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

class TestIso_7_8_5_3 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Note: We cant enable TX to RX feedback here since DUT would
            //       screw us modified bits by transmitting dominant ACK!

            // CAN FD enabled only!
            if (canVersion == CAN_2_0_VERSION ||
                canVersion == CAN_FD_TOLERANT_VERSION)
            {
                testResult = false;
                return false;
            }

            for (int i = 1; i <= nominalBitTiming.sjw; i++)
            {
                // CAN FD frame with bit rate shift
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing ACK negative resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Force driven ACK bit to Dominant.
                 *   3. Shorten CRC delimiter of driven and monitored bits by e.
                 *   4. Force Phase 2 of ACK to Recessive on driven bit!
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);

                Bit *crcDelimiterDriver = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                Bit *crcDelimiterMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                Bit *ackDriver = driverBitFrame->getBitOf(0, BIT_TYPE_ACK);

                crcDelimiterDriver->shortenPhase(PH2_PHASE, i);
                crcDelimiterMonitor->shortenPhase(PH2_PHASE, i);

                for (int j = 0; j < nominalBitTiming.ph2; j++)
                    ackDriver->forceTimeQuanta(j, PH2_PHASE, RECESSIVE);

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