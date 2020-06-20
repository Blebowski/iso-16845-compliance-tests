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
 * @test ISO16845 7.8.9.3
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        CRC delimiter.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          CRC delimiter
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the CRC delimiter bit to dominant from the second
 *             TQ until the beginning of Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT generates a frame with last CRC bit dominant.
 *  The LT forces the CRC delimiter bit to dominant according to elementary
 *  test cases.
 *
 * Response:
 *  The modified CRC delimiter bit shall be sampled as dominant.
 *  The frame is invalid. The CRC delimiter shall be followed by an error
 *  frame.
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

class TestIso_7_8_9_3 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift, Base ID only and
            uint8_t dataByte = 0x49;
            FrameFlags frameFlags = FrameFlags(CAN_FD, BASE_IDENTIFIER,
                                                DATA_FRAME, BIT_RATE_SHIFT, ESI_ERROR_ACTIVE);
            // Frame was empirically debugged to have last bit of CRC in 1!
            goldenFrame = new Frame(frameFlags, 0x1, 50, &dataByte);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("No synchronisation after dominant bit sampled on CRC delimiter bit!");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force CRC delimiter bit to dominant from 2nd TQ till beginning
             *      of Phase Segment 2.
             *   3. Insert Active error frame to monitor from ACK bit further.
             *      Insert Passive error frame to driver bit from ACK bit further.
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *crcDelimiter = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
            crcDelimiter->forceTimeQuanta(1, dataBitTiming.ph1 + dataBitTiming.prop, DOMINANT);

            driverBitFrame->insertPassiveErrorFrame(driverBitFrame->getBitOf(0, BIT_TYPE_ACK));
            monitorBitFrame->insertActiveErrorFrame(monitorBitFrame->getBitOf(0, BIT_TYPE_ACK));

            driverBitFrame->print(true);
            monitorBitFrame->print(true);

            // Push frames to Lower tester, run and check!
            pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
            runLowerTester(true, true);
            checkLowerTesterResult();

            deleteCommonObjects();

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};