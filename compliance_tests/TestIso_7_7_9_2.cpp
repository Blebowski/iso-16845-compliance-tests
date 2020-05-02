/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 3.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.9.2
 * 
 * @brief The purpose of this test is to verify that an IUT will not use any
 *        edge for resynchronization after detection of a recessive to dominant
 *        edge in idle state (after hard synchronization).
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *  Dominant pulses on IDLE bus. Pulse group:
 *      a) First glitch = (Prop_Seg(N) + Phase_Seg1(N) − 2)/2
 *      b) Recessive time = 2 TQ(N)
 *      c) Second glitch = {[Prop_Seg(N) + Phase_Seg1(N) - 2]/2} − 1 minimum
 *         time quantum.
 *      d) Recessive time = 1 TQ(N) + 2 minimum time quantum
 *      e) Third glitch = Prop_Seg(N) + Phase_Seg1(N) – 2
 *  FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 Three dominant glitches separated by recessive TQ(N) times.
 *             The first glitch activates the edge detection of IUT. The next
 *             two glitches cover the TQ(N) position of the configured
 *             Sampling_Point(N) regarding to the first glitch.
 *      Refer to 6.2.3
 *
 * Setup:
 *  No action required, the IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a dominant glitch group according to elementary test cases
 *  for this test case.
 *  Then, the LT waits for 8 bit times to check that no error frame will start
 *  after that.
 * 
 * Response:
 *  The IUT shall remain in the idle state.
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

class TestIso_7_7_9_2 : public test_lib::TestBase
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

            // CAN 2.0 frame, randomize others
            FrameFlags frameFlags = FrameFlags(CAN_2_0);
            goldenFrame = new Frame(frameFlags);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Glitch filtering in idle state - single glitch");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Remove all bits but first 6 from driven frame.
             *   2. Set value of first 5 bits to be corresponding to glitches.
             *      Modify length of bits to each correspond to one glitch/
             *      space between.
             *   3. Remove all bits but first from monitored frame.
             *   4. Insert 9 recessive bits to monitor.
             */
            driverBitFrame->clearBitsFrom(6);

            // Set values
            driverBitFrame->getBit(0)->setBitValue(DOMINANT);
            driverBitFrame->getBit(1)->setBitValue(RECESSIVE);
            driverBitFrame->getBit(2)->setBitValue(DOMINANT);
            driverBitFrame->getBit(3)->setBitValue(RECESSIVE);
            driverBitFrame->getBit(4)->setBitValue(DOMINANT);
            driverBitFrame->getBit(5)->setBitValue(RECESSIVE);

            // Set glitch lengths

            // First reduce other phases, we create glitches it from SYNC!
            for (int i = 0; i < 5; i++)
            {
                printf("Setting bit %d\n", i);
                driverBitFrame->getBit(i)->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);
                driverBitFrame->getBit(i)->shortenPhase(PH1_PHASE, nominalBitTiming.ph2);
                driverBitFrame->getBit(i)->shortenPhase(PROP_PHASE, nominalBitTiming.ph2);
            }

            // Now set to length as in description. SYNC already has length of one!
            driverBitFrame->getBit(0)->lengthenPhase(SYNC_PHASE,
                (nominalBitTiming.prop + nominalBitTiming.ph1 - 2) / 2 - 1);
            
            driverBitFrame->getBit(1)->lengthenPhase(SYNC_PHASE, 1);
            
            driverBitFrame->getBit(2)->lengthenPhase(SYNC_PHASE,
                (nominalBitTiming.prop + nominalBitTiming.ph1 - 2) / 2 - 1);
            driverBitFrame->getBit(2)->getTimeQuanta(0)->shorten(1);

            driverBitFrame->getBit(3)->getTimeQuanta(0)->lengthen(2);
            
            driverBitFrame->getBit(4)->lengthenPhase(SYNC_PHASE,
                nominalBitTiming.prop + nominalBitTiming.ph1 - 3);

            for (int i = 0; i < 9; i++)
                monitorBitFrame->insertBit(Bit(BIT_TYPE_SOF, RECESSIVE,
                    &frameFlags, &nominalBitTiming, &dataBitTiming), 1);

            driverBitFrame->print(true);
            monitorBitFrame->print(true);

            // Push frames to Lower tester, run and check!
            pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
            runLowerTester(true, true);
            checkLowerTesterResult();

            deleteCommonObjects();

            testControllerAgentEndTest(false);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};