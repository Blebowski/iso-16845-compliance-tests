/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 16.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.4
 * 
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        the eighth consecutive dominant bit following the transmission of its
 *        overload flag and after each sequence of additional 8 consecutive
 *        dominant bits.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 * 
 *  CAN FD Enabled
 *      REC, FDF = 1
 * 
 * Elementary test cases:
 *      #1 16 bit dominant
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  After the overload flag sent by the IUT, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 on each eighth dominant bit
 *  after the overload flag.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

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

class TestIso_7_6_4 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/

            int iterCnt;
            int rec;
            int recNew;
            FlexibleDataRate dataRate;

            if (canVersion == CAN_FD_ENABLED_VERSION)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                {
                    testMessage("Common part of test!");
                    dataRate = CAN_2_0;
                } else {
                    testMessage("CAN FD enabled part of test!");
                    dataRate = CAN_FD;
                }

                // CAN 2.0 / CAN FD, randomize others
                FrameFlags frameFlags = FrameFlags(dataRate);
                goldenFrame = new Frame(frameFlags);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // Read REC before scenario
                rec = dutIfc->getRec();

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received.
                 *   2. Force last bit of EOF to Dominant!
                 *   3. Insert Overload frame from first bit of Intermission.
                 *   4. Insert 16 Dominant bits directly after Overload frame
                 *      (from first bit of Overload Delimiter). These bits
                 *      shall be driven on can_tx, but 16 RECESSIVE bits shall
                 *      be monitored on can_tx. 
                 */
                monitorBitFrame->turnReceivedFrame();
                driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);
                driverBitFrame->getBitOf(6, BIT_TYPE_EOF)->setBitValue(DOMINANT);

                monitorBitFrame->insertOverloadFrame(
                    monitorBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));
                driverBitFrame->insertOverloadFrame(
                    driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));

                Bit *ovrDelim = driverBitFrame->getBitOf(0, BIT_TYPE_OVERLOAD_DELIMITER);
                int bitIndex = driverBitFrame->getBitIndex(ovrDelim);

                for (int k = 0; k < 16; k++)
                {
                    driverBitFrame->insertBit(Bit(BIT_TYPE_OVERLOAD_FLAG, DOMINANT,
                        &frameFlags, &nominalBitTiming, &dataBitTiming), bitIndex);
                    monitorBitFrame->insertBit(Bit(BIT_TYPE_OVERLOAD_FLAG, RECESSIVE,
                        &frameFlags, &nominalBitTiming, &dataBitTiming), bitIndex);
                }

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                runLowerTester(true, true);
                checkLowerTesterResult();

                ////////////////////////////////////////////////////////////
                // Receiver will make received frame valid on 6th bit of EOF!
                // Therefore at point where Error occurs, frame was already
                // received OK and should be readable!
                ////////////////////////////////////////////////////////////
                Frame readFrame = this->dutIfc->readFrame();
                if (compareFrames(*goldenFrame, readFrame) == false)
                {
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                }

                recNew = dutIfc->getRec();

                /* 
                 * For first iteration we start from 0 so there will be no
                 * decrement on sucessfull reception! For further increments
                 * there will be alway decrement by 1 and increment by 2*8.
                 */
                int recIncrement;
                if (i == 0)
                    recIncrement = 16;
                else
                    recIncrement = 15;

                if (recNew != (rec + recIncrement))
                {
                    testMessage("DUT REC not as expected. Expected %d, Real %d",
                                    rec + recIncrement, recNew);
                    testResult = false;
                    testControllerAgentEndTest(testResult);
                    return testResult;
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