/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 11.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.10
 * 
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        resynchronization if the value detected at the previous sample point
 *        is the same as the bus value immediately after the edge.
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *      Glitch between 2 dominant sampled bits
 *          FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 One TQ recessive glitch in Phase_Seg2(N).
 *      
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in arbitration field.
 *  At the position [NTQ(N) - Phase_Seg2(N) + 1] time quanta after the falling
 *  edge at the beginning of the stuff bit, the LT changes the value to recessive
 *  for one time quantum according to elementary test cases.
 *  The stuff bit is followed by 5 additional dominant bits.
 * 
 * Response:
 *  The IUT shall respond with an error frame exactly 6 bit times after the
 *  recessive to dominant edge at the beginning of the stuff bit.
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

class TestIso_7_7_10 : public test_lib::TestBase
{
    public:

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);
            
            // Enable TX to RX feedback
            canAgentConfigureTxToRxFeedback(true);
            
            /*****************************************************************
             * Classical CAN / CAN FD Enabled / CAN FD Tolerant are equal
             ****************************************************************/
            
            // CAN 2.0 frame, Base identifier, randomize others
            FrameFlags frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER);

            // Base ID - first 5 bits recessive, next 6 dominant
            // this gives ID with dominant bits after first stuff bit!
            int id = 0b11111000000;
            goldenFrame = new Frame(frameFlags, 0x1, id);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            testMessage("Testing glitch filtering on negative phase error!");

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*goldenFrame,
                &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Monitor frame as if received!
             *   2. Flip NTQ - Ph2 + 1 time quanta of first stuff bit to recessive.
             *   3. Flip second stuff bit to dominant!
             *   4. Insert Active Error flag one bit after 2nd stuff bit!
             *      Insert Passive Error flag to driver so that it transmitts all
             *      recessive!
             */
            monitorBitFrame->turnReceivedFrame();

            Bit *firstStuffBit = driverBitFrame->getStuffBit(0);
            int tqPosition = firstStuffBit->getLenTimeQuanta() -
                             nominalBitTiming.ph2 + 1;
            firstStuffBit->getTimeQuanta(tqPosition - 1)->forceValue(RECESSIVE);

            Bit *secondStuffBit = driverBitFrame->getStuffBit(1);
            secondStuffBit->setBitValue(DOMINANT);
            int index = driverBitFrame->getBitIndex(secondStuffBit);

            monitorBitFrame->insertActiveErrorFrame(index + 1);
            driverBitFrame->insertPassiveErrorFrame(index + 1);

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