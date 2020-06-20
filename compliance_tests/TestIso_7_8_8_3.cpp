/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.8.3
 *
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there is an additional recessive
 *        to dominant edge between two sample points where the first edge comes
 *        before the synchronization segment on bit position ACK.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Bit start with negative offset and glitch between synchronization
 *      segment and sample point.
 *          ACK
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *      #1 The LT reduce the length of CRC delimiter bit by one TQ(D) and the
 *         LT force the second TQ of ACK bit to Recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
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

class TestIso_7_8_8_3 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Note: In this TC TX to RX feedback cant be enabled, since DUT
            //       would corrupt test pattern by IUT in ACK field!

            // CAN FD enabled only!
            if (canVersion == CAN_2_0_VERSION ||
                canVersion == CAN_FD_TOLERANT_VERSION)
            {
                testResult = false;
                return false;
            }

            // CAN FD frame with bit rate shift
            FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
            goldenFrame = new Frame(frameFlags);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Testing ACK bit glitch filtering on negative phase error");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten CRC delimiter by 1 TQ in both driven and monitored
             *      frames.
             *   3. Force 2nd TQ of driven ACK bit to Recessive.
             *   4. Force whole Phase 2 of ACK bit to Recessive.
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *crcDelimDriver = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
            Bit *crcDelimMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
            Bit *ackBit = driverBitFrame->getBitOf(0, BIT_TYPE_ACK);

            // ACK must be sent dominant since TX/RX feedback is not turned on!
            ackBit->setBitValue(DOMINANT);

            crcDelimDriver->shortenPhase(PH2_PHASE, 1);
            crcDelimMonitor->shortenPhase(PH2_PHASE, 1);

            ackBit->forceTimeQuanta(1, RECESSIVE);
            ackBit->forceTimeQuanta(0, nominalBitTiming.ph2 - 1, PH2_PHASE, RECESSIVE);

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

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};