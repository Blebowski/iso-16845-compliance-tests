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
 * @test ISO16845 7.8.2.2
 * 
 * @brief The purpose of this test is to verify that the IUT makes a hard
 *        synchronization when receiving an early recessive to dominant edge
 *        between FDF and “res” bit by e, where:
 *            e = Phase_Seg2(N)
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(N) configuration as available by IUT.
 *          SJW(N) = 1
 *          res
 *          FDF = 1
 *          BRS = 0
 *          ESI = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The LT generates a valid frame with shortened FDF bit by an amount
 *             of e = Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 * 
 *  The LT sets the last Phase_Seg2(D) TQ of the dominant BRS bit to recessive.
 * 
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The hard synchronization shall correct the maximum phase error as defined
 *  in ISO 11898-1.
 *  The frame is valid. No error flag shall occur. The bit rate will not switch
 *  for data phase.
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

class TestIso_7_8_2_2 : public test_lib::TestBase
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
            FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_DONT_SHIFT);
            goldenFrame = new Frame(frameFlags);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Testing 'res' bit hard-sync with negative phase error");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten PH2 of FDF/EDL bit to 0 (both driven and monitored
             *      frames since DUT shall Hard synchronize)
             *   3. Force TSEG2 of BRS to Recessive on driven frame!
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *edlBitDriver = driverBitFrame->getBitOf(0, BIT_TYPE_EDL);
            Bit *edlBitMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_EDL);
            Bit *brsBit = driverBitFrame->getBitOf(0, BIT_TYPE_BRS);

            edlBitDriver->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);
            edlBitMonitor->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);

            for (int j = 0; j < dataBitTiming.ph2; j++)
                brsBit->getTimeQuanta(PH2_PHASE, j)->forceValue(RECESSIVE);

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