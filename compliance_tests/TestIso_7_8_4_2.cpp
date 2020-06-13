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
 * @test ISO16845 7.8.4.2
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e > SJW(D) on bit position DATA.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          ESI = 1
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              e ∈ {[SJW(D) + 1], [NTQ(D) − Phase_Seg2(D) − 1]{
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  Then, the recessive to dominant edge before this dominant stuff bit shall
 *  be delayed by additional e TQ(D)’s of recessive value at the beginning of
 *  this stuff bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed stuff bit to rece-
 *  ssive. This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after
 *  sampling point.
 *
 *  The LT forces a part of Phase_Seg2(D) of the delayed ESI bit to recessive.
 *  This recessive part of Phase_seg2 start at SJW(D) − 1 TQ(D) after sampling
 *  point.
 *
 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The wrong value of stuff bit shall cause an error flag.
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

class TestIso_7_8_4_2 : public test_lib::TestBase
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

            for (int i = dataBitTiming.sjw + 1; i < upperTh; i++)
            {
                // CAN FD frame with bit rate shift
                uint8_t dataByte = 0x7F;
                FrameFlags frameFlags = FrameFlags(CAN_FD, BIT_RATE_SHIFT);
                goldenFrame = new Frame(frameFlags, 0x1, &dataByte);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                testMessage("Testing data byte positive resynchronisation with phase error: %d", i + 1);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*goldenFrame,
                    &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. Turn monitor frame as if received!
                 *   2. Force first e time quantas of 7-th data bit to Recessive.
                 *      This bit should be dominant stuff bit.
                 *   3. Force 7-th data bit from SJW - 1 after sample point till the end to
                 *      Recessive.
                 *   4. Lengthen monitored 7-th data bit by SJW (this correspond to
                 *      DUTs resync. by SJW).
                 *   5. Insert active error frame from 8-th data bit further to monitored
                 *      frame. Insert passive error frame to driven frame!
                 */
                monitorBitFrame->turnReceivedFrame();

                Bit *driverStuffBit = driverBitFrame->getBitOf(6, BIT_TYPE_DATA);
                Bit *monitorStuffBit = monitorBitFrame->getBitOf(6, BIT_TYPE_DATA);

                // One bit after stuff bit will be recessive due to data byte. Insert
                // passive error frame from one bit further so that model does not modify
                // the stuff bit due to insertion of error frame after bit in data bit rate!
                Bit *driverNextBit = driverBitFrame->getBitOf(8, BIT_TYPE_DATA);
                Bit *monitorNextBit = monitorBitFrame->getBitOf(7, BIT_TYPE_DATA);

                for (int j = 0; j < i; j++)
                    driverStuffBit->forceTimeQuanta(j, RECESSIVE);
                for (int j = dataBitTiming.sjw - 1; j < dataBitTiming.ph2; j++)
                    driverStuffBit->forceTimeQuanta(j, PH2_PHASE, RECESSIVE);

                monitorStuffBit->lengthenPhase(SYNC_PHASE, dataBitTiming.sjw);

                driverBitFrame->insertPassiveErrorFrame(driverNextBit);
                monitorBitFrame->insertActiveErrorFrame(monitorNextBit);

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