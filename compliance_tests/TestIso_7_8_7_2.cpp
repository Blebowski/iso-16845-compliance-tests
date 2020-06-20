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
 * @test ISO16845 7.8.7.2
 *
 * @brief The purpose of this test is to verify that there is only one synchro-
 *        nization within 1 bit time if there are additional recessive to do-
 *        minant edges between synchronization segment and sample point on bit
 *        position DATA.
 *
 *        The test also verifies that an IUT is able to synchronize on a
 *        minimum duration pulse obeying to the synchronization rules.
 * 
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          Glitch between synchronization segment and sample point.
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the second TQ of a dominant stuff bit to recessive.
 *      
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *  Additionally, the Phase_Seg2(D) of this dominant stuff bit shall be forced
 *  to recessive.
 *  
 * Response:
 *  The modified stuff bit shall be sampled as dominant.
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

class TestIso_7_8_7_2 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift
            uint8_t dataByte = 0x7F;
            FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
            goldenFrame = new Frame(frameFlags, 0x1, &dataByte);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Testing gltich filtering in data bit-rate on data byte");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force last e TQ of 6-th bit of data field by e TQ to
             *      dominant. This should be a bit before stuff bit.
             *   3. Force PH2 of 7-th bit of data field to Recessive. This
             *      should be a stuff bit.
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *driverStuffBit = driverBitFrame->getBitOf(6, BIT_TYPE_DATA);
            driverStuffBit->forceTimeQuanta(1, RECESSIVE);
            driverStuffBit->forceTimeQuanta(0, dataBitTiming.ph2 - 1, PH2_PHASE, RECESSIVE);

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