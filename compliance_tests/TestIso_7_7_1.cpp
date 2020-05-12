/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 2.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.1
 * 
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT shortens a dominant stuff bit in arbitration field by an amount of 
 *  Phase_Seg2(N) and then later shortens another dominant stuff bit by an
 *  amount of [Phase_Seg2(N) + 1] according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame on the bit position following the
 *  second shortened stuff bit.
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

class TestIso_7_7_1 : public test_lib::TestBase
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

            // CAN 2.0 frame, Base identifier, randomize others
            FrameFlags frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER);

            // Base ID full of 1s
            int id = pow(2,11) - 1;
            goldenFrame = new Frame(frameFlags, 0x1, id);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Sample point test.");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Shorten 6-th bit (1st stuff bit) of driven frame by PhaseSeg2.
             *   3. Shorten 12-th bit (2nd stuff bit) of driven frame still
             *      in Base ID by PhaseSeg2 + 1.
             *   4. Correct lenght of one of the monitored bits since second
             *      stuff bit causes negative re-synchronization.
             *   5. Insert expected Error frame from next bit (11-th bit of Base ID)
             *      to monitored frame. Don't insert this as driven frame as this
             *      would cause dominant bit to be received by DUT just after
             *      PH2 of Second stuff bit was shortened!
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *firstStuffBit = driverBitFrame->getStuffBit(0);
            firstStuffBit->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);

            Bit *secStuffBit = driverBitFrame->getStuffBit(1);
            secStuffBit->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);
            BitPhase prevPhase = secStuffBit->prevBitPhase(PH2_PHASE);
            secStuffBit->shortenPhase(prevPhase, 1);

            /* Compensate the monitored frame as if negative resynchronisation
               happend */
            int resyncAmount = nominalBitTiming.ph2;
            if (nominalBitTiming.sjw < resyncAmount)
                resyncAmount = nominalBitTiming.sjw;
            monitorBitFrame->getBitOf(11, BIT_TYPE_BASE_ID)->shortenPhase(
                PH2_PHASE, resyncAmount);

            /* 5 + Stuff + 5 + Stuff = 12 bits. We need to insert error frame
               from 13-th bit on! */
            int bitIndex = driverBitFrame->getBitIndex(
                            driverBitFrame->getBitOf(12, BIT_TYPE_BASE_ID));

            // Expected error frame on monitor (from start of bit after stuff bit)
            monitorBitFrame->insertActiveErrorFrame(bitIndex);

            /* 
             * For DUT to succesfully transmitt error frame, also driver must
             * transmit it! But we don't want to transmitt it right away to
             * avoid wrong synchronisation of DUT due to received start of
             * error frame! So we insert it one bit later! Since driven frame
             * is shortened, DUT will still catch it as first bit during
             * monitored frame!
             */
            driverBitFrame->insertActiveErrorFrame(bitIndex + 1);

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