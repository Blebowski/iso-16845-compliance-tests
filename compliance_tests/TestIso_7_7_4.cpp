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
 * @test ISO16845 7.7.4
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e > SJW(N).
 *
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *      
 *      #1 The values tested for e are measured in time quanta with
 *          e ∈ [SJW(N) + 1], [NTQ(N) − Phase_Seg2(N) − 1].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT delays a dominant stuff bit in arbitration field by an amount of
 *  e time quanta and shortens the same bit by an amount of [Phase_Seg2(N) 
 *  + 1TQ + e − SJW(N)] according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame 1 bit time – [e − SJW(N)] time quanta
 *  after the recessive to dominant edge of the delayed stuff bit.
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

class TestIso_7_7_4 : public test_lib::TestBase
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

            for (int e = nominalBitTiming.sjw + 1;
                 e <= nominalBitTiming.prop + nominalBitTiming.ph1;
                 e++)
            {
                // CAN 2.0 frame, Base identifier, randomize others
                FrameFlags frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER);

                // Base ID full of 1s, 5th will be dominant stuff bit!
                int id = pow(2,11) - 1;
                goldenFrame = new Frame(frameFlags, 0x1, id);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing positive phase error: %d", e);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Prolong TSEG2 of driven bit before the stuff bit on 5th
                 *      bit of identifier (delay stuff bit) by e. Prolong TSEG1
                 *      of monitored stuff bit by SJW. This corresponds to resync.
                 *      by SJW!
                 *   2. Force whole TSEG2 and last time quanta of TSEG1 to
                 *      Recessive. This corresponds to shortening the bit by
                 *      TSEG2 + 1.
                 *   3. Insert Active Error frame to both driven and monitored
                 *      frame from next bit!
                 */
                monitorBitFrame->turnReceivedFrame();
                Bit *beforeStuffBit = driverBitFrame->getBitOf(3, BIT_TYPE_BASE_ID);
                beforeStuffBit->lengthenPhase(PH2_PHASE, e);

                // Monitor bit as if node re-synchronised by SJW!
                Bit *monitorStuffBit = monitorBitFrame->getStuffBit(0);
                monitorStuffBit->lengthenPhase(SYNC_PHASE, nominalBitTiming.sjw);

                Bit *driverStuffBit = driverBitFrame->getStuffBit(0);
                for (int j = 0; j < nominalBitTiming.ph2; j++)
                    driverStuffBit->forceTimeQuanta(j, PH2_PHASE, RECESSIVE);

                BitPhase prevPhase = driverStuffBit->prevBitPhase(PH2_PHASE);
                int toBeShortened = e - nominalBitTiming.sjw + 1;
                
                int shortened = driverStuffBit->shortenPhase(prevPhase, toBeShortened);

                if (shortened < toBeShortened)
                {
                    prevPhase = driverStuffBit->prevBitPhase(prevPhase);
                    driverStuffBit->shortenPhase(prevPhase, toBeShortened - shortened);
                }

                int index = driverBitFrame->getBitIndex(driverStuffBit);
                driverBitFrame->insertActiveErrorFrame(index + 1);
                monitorBitFrame->insertActiveErrorFrame(index + 1);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

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