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
 * @test ISO16845 7.8.9.1
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        BRS.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          BRS = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the first two TQ(N) and the complete Phase_Seg2(N)
 *             of BRS bit to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant BRS bit.
 *  The LT inverts parts of BRS bit according to elementary test cases.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur. The bit rate will not
 *  switch for data phase.
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

class TestIso_7_8_9_1 : public test_lib::TestBase
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

            // CAN FD frame with bit rate shift, Dont shift bit rate
            // Here we have to set Bit rate dont shift because we intend to
            // get BRS dominant, so bit rate should not be shifted!
            FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_DONT_SHIFT);
            goldenFrame = new Frame(frameFlags);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("No synchronisation after dominant bit sampled on BRS bit!");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Flip BRS value to dominant.
             *   3. Force first two TQ of BRS and Phase 2 of BRS to
             *      Recessive. This should cause resynchronisation edge
             *      with phase error 2, but DUT shall ignore it and not
             *      resynchronize because previous bit (r0) was Dominant!
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *brsBit = driverBitFrame->getBitOf(0, BIT_TYPE_BRS);
            
            brsBit->setBitValue(DOMINANT);
            
            brsBit->forceTimeQuanta(0, RECESSIVE);
            brsBit->forceTimeQuanta(1, RECESSIVE);

            // Force all TQ of PH2 as if no shift occured (this is what frame
            // was generated with)
            brsBit->forceTimeQuanta(0, nominalBitTiming.ph2 - 1, PH2_PHASE, RECESSIVE);

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