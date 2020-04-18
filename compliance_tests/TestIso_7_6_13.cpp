/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.13
 * 
 * @brief This test verifies that a receiver increases its REC by 1 when
 *        detecting a form error on a bit of the overload delimiter it is
 *        transmitting.
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
 *      #1 the second bit of the overload delimiter is corrupted;
 *      #2 the seventh bit of the overload delimiter is corrupted.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  The LT corrupts 1 bit of the overload delimiter according to elementary
 *  test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 1 after reception of the
 *  dominant bit sent by the LT.
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

class TestIso_7_6_13 : public test_lib::TestBase
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

                for (int j = 0; j < 2; j++)
                {
                    // CAN 2.0 / CAN FD, randomize others
                    FrameFlags frameFlags = FrameFlags(dataRate);
                    goldenFrame = new Frame(frameFlags);
                    goldenFrame->randomize();
                    testBigMessage("Test frame:");
                    goldenFrame->print();

                    // Read REC before scenario
                    rec = dutIfc->getRec();

                    int bitToCorrupt;
                    if (j == 0)
                        bitToCorrupt = 2;
                    else
                        bitToCorrupt = 7;

                    testMessage("Forcing Overload delimiter bit %d to recessive",
                                bitToCorrupt);

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
                     *   4. Flip n-th bit of Overload delimiter to DOMINANT!
                     *   5. Insert Active Error frame to both monitored and driven
                     *      frame!
                     */
                    monitorBitFrame->turnReceivedFrame();
                    driverBitFrame->getBitOf(0, BIT_TYPE_ACK)->setBitValue(DOMINANT);
                    driverBitFrame->getBitOf(6, BIT_TYPE_EOF)->setBitValue(DOMINANT);

                    monitorBitFrame->insertOverloadFrame(
                        monitorBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));
                    driverBitFrame->insertOverloadFrame(
                        driverBitFrame->getBitOf(0, BIT_TYPE_INTERMISSION));

                    // Force n-th bit of Overload Delimiter to Dominant
                    Bit *bit = driverBitFrame->getBitOf(bitToCorrupt - 1, BIT_TYPE_OVERLOAD_DELIMITER);
                    int bitIndex = driverBitFrame->getBitIndex(bit);
                    bit->setBitValue(DOMINANT);

                    // Insert Error flag from one bit further, both driver and monitor!
                    driverBitFrame->insertActiveErrorFrame(bitIndex + 1);
                    monitorBitFrame->insertActiveErrorFrame(bitIndex + 1);

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

                    // For first iteration we start from 0 so there will be no
                    // decrement on sucessfull reception! So there will be only
                    // increment by 1. On each next step, there will be decrement
                    // by 1 (succesfull reception) and increment by 1 due to
                    // form error on overload delimiter!
                    int recIncrement;
                    if (i == 0 && j == 0)
                        recIncrement = 1;
                    else
                        recIncrement = 0;

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
            }

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};