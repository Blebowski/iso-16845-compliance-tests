/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 13.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.1
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW(D) on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          ESI = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *              |e| ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant ESI bit.
 *  The LT shortened the BRS bit by an amount of |e| TQ according to ele-
 *  mentary test cases.
 *  Additionally, the ESI bit shall be forced to recessive value from
 *  [Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) − e] up to end of bit.
 *
 * Response:
 *  The modified ESI bit shall be sampled as dominant.
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

class TestIso_7_8_5_1 : public test_lib::TestBase
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

            int upperTh = dataBitTiming.ph1 + dataBitTiming.prop + 1;

            for (int i = 1; i <= dataBitTiming.sjw; i++)
            {
                // CAN FD frame with bit rate shift, ESI = Dominant
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT, ESI_ERROR_ACTIVE);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing ESI negative resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Shorten PH2 of BRS by e.
                 *   3. Force ESI to Recessive from Sync+Prop+Ph1-e till the
                 *      end of bit.
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *brsBit = driverBitFrame->getBitOf(0, BIT_TYPE_BRS);
                Bit *esiBit = driverBitFrame->getBitOf(0, BIT_TYPE_ESI);
                Bit *brsBitMonitor = monitorBitFrame->getBitOf(0, BIT_TYPE_BRS);


                /**************************************************************
                 * These modifications are how I think this was meant!!
                 **************************************************************/
                brsBit->shortenPhase(PH2_PHASE, i);
                brsBitMonitor->shortenPhase(PH2_PHASE, i);

                for (int j = 0; j < dataBitTiming.ph2; j++)
                    esiBit->forceTimeQuanta(j, PH2_PHASE, RECESSIVE);


                /**************************************************************
                 * This is exactly how TC describes it in ISO16845-1 2016 and I
                 * think it is wrong!!!
                 **************************************************************/
                // Shorten BRS by e
                // brsBit->shortenPhase(PH2_PHASE, i);

                //int startTq = 1 + dataBitTiming.prop + dataBitTiming.ph1 - i;
                //for (int j = startTq; j < brsBit->getLenTimeQuanta(); j++)
                //    esiBit->forceTimeQuanta(j, RECESSIVE);


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