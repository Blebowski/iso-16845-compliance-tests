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
 * @test ISO16845 7.8.3.3
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e ≤ SJW(D) on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          CRC: LSB = 1
 *          CRC delimiter
 *          FDF = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             e ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a test frame with a recessive bit value at last bit of CRC.
 *  The LT forces the CRC delimiter to dominant bit value.
 *  Then, the recessive to dominant edge between LSB of CRC and CRC delimiter
 *  shall be delayed by additional e TQ(D)’s of recessive value at the
 *  beginning of CRC delimiter bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed CRC delimiter bit to
 *  recessive. This recessive part of Phase_seg2 start at e − 1 TQ(D) after
 *  sampling point.
 * 
 * Response:
 *  The modified CRC delimiter bit shall be sampled as recessive.
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

class TestIso_7_8_3_3 : public test_lib::TestBase
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

            for (int i = 0; i < dataBitTiming.sjw; i++)
            {
                // CAN FD frame with bit rate shift, Base ID only and
                uint8_t dataByte = 0x55;
                FrameFlags frameFlags = FrameFlags(CAN_FD, BASE_IDENTIFIER,
                                                   DATA_FRAME, BIT_RATE_SHIFT, ESI_ERROR_ACTIVE);
                // Frame was empirically debugged to have last bit of CRC in 1!
                goldenFrame = new Frame(frameFlags, 0x1, 50, &dataByte);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing CRC Delimiter positive resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Force CRC delimiter of driven frame to dominant.
                 *   3. Force first e bits of CRC delimiter to Recessive
                 *      (This delays dominant to recessive edge by e T!)
                 *   4. Shorten PH2 of CRC Delimiter to 0 since this-one
                 *      is in multiples of nominal Time quanta. Lengthen
                 *      PH1 (still in data time quanta), by e - 1. This
                 *      has equal effect as forcing the bit to Recessive
                 *      e - 1 after sample point. Next bit is ACK which
                 *      is transmitted recessive by driver.
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *crcDelimiter = driverBitFrame->getBitOf(0, BIT_TYPE_CRC_DELIMITER);
                crcDelimiter->setBitValue(DOMINANT);

                for (int j = 0; j < i + 1; j++)
                    crcDelimiter->forceTimeQuanta(j, RECESSIVE);

                crcDelimiter->shortenPhase(PH2_PHASE, nominalBitTiming.ph2);
                BitPhase phase = crcDelimiter->prevBitPhase(PH2_PHASE);
                crcDelimiter->lengthenPhase(phase, i);

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