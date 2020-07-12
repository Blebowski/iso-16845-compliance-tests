/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 09.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.3
 *
 * @brief This test verifies the capability of the IUT to manage the arbitration
 *        mechanism on every bit position in a base format frame it is
 *        transmitting.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD Enabled
 *
 * Test variables:
 *      ID
 *      DLC
 *      FDF = 0
 *
 * Elementary test cases:
 *      For an OPEN device, there are, at most, 11 elementary tests to perform.
 *          Transmitted frame
 *    ID      RTR/RRS         DATA      Description of the     Number of elementary
 *                           field     concerned arbitration          tests
 *                                            bit               
 *  0x7EF        0          No Data     Collision on all bits          10
 *                                           equal to 1.
 *  0x010        0          No Data     Collision on all bits          1
 *                                           equal to 1.
 *  For a SPECIFIC device, all possible possibilities of transmitting a recessive
 *  arbitration bit shall be considered.
 *  
 *  For CAN FD enabled test, the RTR is represented by RRS and transmitted as 0.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT forces a recessive
 *  bit in the arbitration field to the dominant state according to the table in
 *  elementary test cases and continues to send a valid frame.
 *
 * Response:
 *  The IUT shall become receiver when sampling the dominant bit sent by the LT.
 *  As soon as the bus is idle, the IUT shall restart the transmission of the
 *  frame. The IUT shall not generate any error flag during the test.
 *  The content of the frame shall match the LT request.
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

class TestIso_8_1_3 : public test_lib::TestBase
{
    public:

        /*****************************************************************
         * Common part of test - 10 iterations with 0x7EF + 1 with 0x010 
         ****************************************************************/
        int run_can_2_0()
        {
            for (int i = 0; i < 11; i++)
            {
                int dut_id = 0x7EF;
                int lt_id = dut_id;

                testMessage("CAN 2.0: Invoking arbitration lost %d-th bit of Base id", i + 1);

                /* 
                 * Data byte does not matter from test meaning point. Use only one byte
                 * to keep the test short. However, different data byte can cause that
                 * CRC contains stuff bit, therefore monitor sequence needs to be
                 * compensated on different bit!
                 */
                uint8_t dataByte = 0x55;

                // 7-th bit is dominant -> No arbitration lost!
                if (i == 6)
                    continue;

                // LT Identifier for golden frame must have Dominant in n-th bit because
                // this is what DUT will see!
                lt_id &= ~(1 << (10 - i));

                // Golden frame - this is what LT will transmit
                // CAN 2.0 Frame, Base ID only, Data frame, one byte is enough
                FrameFlags frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER, DATA_FRAME);
                goldenFrame = new Frame(frameFlags, 0x1, lt_id, &dataByte);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // DUT frame - will be sent by DUT
                Frame *dutFrame = new Frame(frameFlags, 0x1, dut_id, &dataByte);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*dutFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);

                BitFrame *secDriverBitFrame = new BitFrame(*dutFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);
                BitFrame *secMonitorBitFrame = new BitFrame(*dutFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. First i bits of identifier are equal between driven and monitored
                 *      frame. On i bit, LT drives dominant bit, but DUT transmitts recessive.
                 *      So from i+1 bit, LT should transmit recessive only. This corresponds
                 *      to DUT loosing arbitration on i-th bit!
                 *   2. Turn 2nd frame as received (there LT is not sending anything)!
                 *   3. Append 2nd frame after the first one. This represents exactly as if
                 *      DUT will retransmitt frame after intermission.
                 *   4. Monitor frame is calculated from frame which DUT would sent and then
                 *      arbitration is lost on i-th bit! LT frame is different than monitored
                 *      frame, because LT frame can have less stuff bits due to flipped
                 *      dominant bit. In such cases, monitored frame is one bit longer, and
                 *      it needs to be compensated! This happends in all test iterations where
                 *      insertion of dominant bit by LT causes dropping of stuff bit in
                 *      comparison with nominal bit rate.
                 */
                Bit *lostArbBit = monitorBitFrame->getBitOfNoStuffBits(i, BIT_TYPE_BASE_ID);
                monitorBitFrame->looseArbitration(lostArbBit);

                secDriverBitFrame->turnReceivedFrame();

                driverBitFrame->appendBitFrame(secDriverBitFrame);
                monitorBitFrame->appendBitFrame(secMonitorBitFrame);

                /* Compensation
                 *  Raw:            11111101111
                 *  Stuffed:        111110101111
                 *  
                 *  Index Modified:
                 *  0:              011111001111
                 *  1:              10111101111     -> Should compensate
                 *  2:              11011101111     -> Should compensate
                 *  3:              11101101111     -> Should compensate
                 *  4:              11110101111     -> Should compensate, but does
                 *                                     not since CRC compensates by
                 *                                     additional stuff bit.
                 *  5:              111110001111
                 *  5:              111110101111
                 *  6:              11111101111     -> Skipped
                 *  7:              111110100111
                 *  8:              111110101011
                 *  9:              111110101101
                 *  10:             111110101110
                 */

                // Compensation by removing bits                  
                if (i == 1 || i == 2 || i == 3)
                    monitorBitFrame->removeBit(monitorBitFrame->getBitOf(0, BIT_TYPE_CRC));

                // Compensation by adding bits (changed bit caused shorter CRC than with
                // normal bit value)
                if (i == 5)
                    monitorBitFrame->insertBit(Bit(BIT_TYPE_R0, RECESSIVE, &frameFlags,
                                                   &nominalBitTiming, &dataBitTiming), 13);

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, insert to DUT, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                startDriverAndMonitor();

                testMessage("Sending frame via DUT!");
                this->dutIfc->sendFrame(dutFrame);
                testMessage("Sent frame via DUT!");

                waitForDriverAndMonitor();
                checkLowerTesterResult();

                deleteCommonObjects();
                delete secDriverBitFrame;
                delete secMonitorBitFrame;
            }

            /*****************************************************************
             * Last elementary test-case of nominal bit rate: id = 0x010
             ****************************************************************/
            int dut_id = 0x010;
            int lt_id = dut_id;

            uint8_t dataByte = 0x55;

            testMessage("CAN 2.0: Invoking arbitration lost 7 bit of Base id");

            // LT Identifier for golden frame must have Dominant in 7-th bit because
            // this is what DUT will see!
            lt_id &= ~(1 << 4);

            // Golden frame - this is what LT will transmit
            // CAN 2.0 Frame, Base ID only, Data frame, one byte is enough
            FrameFlags frameFlags = FrameFlags(CAN_2_0, BASE_IDENTIFIER, DATA_FRAME);
            goldenFrame = new Frame(frameFlags, 0x1, lt_id, &dataByte);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            // DUT frame - will be sent by DUT
            Frame *dutFrame = new Frame(frameFlags, 0x1, dut_id, &dataByte);

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*dutFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);

            BitFrame *secDriverBitFrame = new BitFrame(*dutFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);
            BitFrame *secMonitorBitFrame = new BitFrame(*dutFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Loose arbitration on monitored frame, 7-th bit!
             *   2. Turn 2nd frame as received (there LT is not sending anything)!
             *   3. Append 2nd frame after the first one. This represents exactly as if
             *      DUT will retransmitt frame after intermission.
             *   4. Do compensation, since due to flipping we loose one stuff bit!
             */
            Bit *lostArbBit = monitorBitFrame->getBitOfNoStuffBits(6, BIT_TYPE_BASE_ID);
            monitorBitFrame->looseArbitration(lostArbBit);

            secDriverBitFrame->turnReceivedFrame();

            driverBitFrame->appendBitFrame(secDriverBitFrame);
            monitorBitFrame->appendBitFrame(secMonitorBitFrame);

            monitorBitFrame->removeBit(monitorBitFrame->getBitOf(0, BIT_TYPE_DATA));

            driverBitFrame->print(true);
            monitorBitFrame->print(true);

            // Push frames to Lower tester, insert to DUT, run and check!
            pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
            startDriverAndMonitor();

            testMessage("Sending frame via DUT!");
            this->dutIfc->sendFrame(dutFrame);
            testMessage("Sent frame via DUT!");

            waitForDriverAndMonitor();
            checkLowerTesterResult();

            deleteCommonObjects();
            delete secDriverBitFrame;
            delete secMonitorBitFrame;
        }


        /*****************************************************************
         * CAN FD Enabled part of the test - 10 iterations with 0x7EF
         ****************************************************************/
        int run_can_fd()
        {
            for (int i = 0; i < 11; i++)
            {
                int dut_id = 0x7EF;
                int lt_id = dut_id;

                testMessage("CAN FD: Invoking arbitration lost %d-th bit of Base id", i + 1);

                /* 
                 * Data byte does not matter from test meaning point. Use only one byte
                 * to keep the test short. However, different data byte can cause that
                 * CRC contains stuff bit, therefore monitor sequence needs to be
                 * compensated on different bit!
                 */
                uint8_t dataByte = 0x55;

                // 7-th bit is dominant -> No arbitration lost!
                if (i == 6)
                    continue;

                // LT Identifier for golden frame must have Dominant in n-th bit because
                // this is what DUT will see!
                lt_id &= ~(1 << (10 - i));

                // Golden frame - this is what LT will transmit
                // CAN FD Frame, Base ID only, Data frame, one byte is enough
                FrameFlags frameFlags = FrameFlags(CAN_FD, BASE_IDENTIFIER, DATA_FRAME,
                                                    BIT_RATE_SHIFT, ESI_ERROR_ACTIVE);
                goldenFrame = new Frame(frameFlags, 0x1, lt_id, &dataByte);
                goldenFrame->randomize();
                testBigMessage("Test frame:");
                goldenFrame->print();

                // DUT frame - will be sent by DUT
                Frame *dutFrame = new Frame(frameFlags, 0x1, dut_id, &dataByte);

                // Convert to Bit frames
                driverBitFrame = new BitFrame(*goldenFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);
                monitorBitFrame = new BitFrame(*dutFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);

                BitFrame *secDriverBitFrame = new BitFrame(*dutFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);
                BitFrame *secMonitorBitFrame = new BitFrame(*dutFrame,
                                                &this->nominalBitTiming, &this->dataBitTiming);

                /**
                 * Modify test frames:
                 *   1. First i bits of identifier are equal between driven and monitored
                 *      frame. On i bit, LT drives dominant bit, but DUT transmitts recessive.
                 *      So from i+1 bit, LT should transmit recessive only. This corresponds
                 *      to DUT loosing arbitration on i-th bit!
                 *   2. Turn 2nd frame as received (there LT is not sending anything)!
                 *   3. Append 2nd frame after the first one. This represents exactly as if
                 *      DUT will retransmitt frame after intermission.
                 *   4. Do compensation for changed bits which caused drop of stuff bits!
                 */
                Bit *lostArbBit = monitorBitFrame->getBitOfNoStuffBits(i, BIT_TYPE_BASE_ID);
                monitorBitFrame->looseArbitration(lostArbBit);

                secDriverBitFrame->turnReceivedFrame();

                driverBitFrame->appendBitFrame(secDriverBitFrame);
                monitorBitFrame->appendBitFrame(secMonitorBitFrame);

                /* Compensation
                 *  Raw:            11111101111
                 *  Stuffed:        111110101111
                 *  
                 *  Index Modified:
                 *  0:              011111001111
                 *  1:              10111101111     -> Should compensate
                 *  2:              11011101111     -> Should compensate
                 *  3:              11101101111     -> Should compensate
                 *  4:              11110101111     -> Should compensate
                 *  5:              111110001111
                 *  5:              111110101111
                 *  6:              11111101111     -> Skipped
                 *  7:              111110100111
                 *  8:              111110101011
                 *  9:              111110101101
                 *  10:             111110101110
                 */

                // Compensation by removing bits                  
                if (i == 1 || i == 2 || i == 3 || i == 4)
                    monitorBitFrame->removeBit(monitorBitFrame->getBitOf(0, BIT_TYPE_R0));

                driverBitFrame->print(true);
                monitorBitFrame->print(true);

                // Push frames to Lower tester, insert to DUT, run and check!
                pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
                startDriverAndMonitor();

                testMessage("Sending frame via DUT!");
                this->dutIfc->sendFrame(dutFrame);
                testMessage("Sent frame via DUT!");

                waitForDriverAndMonitor();
                checkLowerTesterResult();

                deleteCommonObjects();
                delete secDriverBitFrame;
                delete secMonitorBitFrame;
            }

            /*****************************************************************
             * Last elementary test-case of data bit rate: id = 0x010
             ****************************************************************/
            int dut_id = 0x010;
            int lt_id = dut_id;

            uint8_t dataByte = 0x55;

            testMessage("CAN FD: Invoking arbitration lost 7 bit of Base id");

            // LT Identifier for golden frame must have Dominant in 7-th bit because
            // this is what DUT will see!
            lt_id &= ~(1 << 4);

            // Golden frame - this is what LT will transmit
            // CAN 2.0 Frame, Base ID only, Data frame, one byte is enough
            FrameFlags frameFlags = FrameFlags(CAN_FD, BASE_IDENTIFIER, DATA_FRAME,
                                                BIT_RATE_SHIFT, ESI_ERROR_ACTIVE);
            goldenFrame = new Frame(frameFlags, 0x1, lt_id, &dataByte);
            goldenFrame->randomize();
            testBigMessage("Test frame:");
            goldenFrame->print();

            // DUT frame - will be sent by DUT
            Frame *dutFrame = new Frame(frameFlags, 0x1, dut_id, &dataByte);

            // Convert to Bit frames
            driverBitFrame = new BitFrame(*goldenFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);
            monitorBitFrame = new BitFrame(*dutFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);

            BitFrame *secDriverBitFrame = new BitFrame(*dutFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);
            BitFrame *secMonitorBitFrame = new BitFrame(*dutFrame,
                                            &this->nominalBitTiming, &this->dataBitTiming);

            /**
             * Modify test frames:
             *   1. Loose arbitration on monitored frame, 7-th bit!
             *   2. Turn 2nd frame as received (there LT is not sending anything)!
             *   3. Append 2nd frame after the first one. This represents exactly as if
             *      DUT will retransmitt frame after intermission.
             *   4. No compensation needed in this case!
             */
            Bit *lostArbBit = monitorBitFrame->getBitOfNoStuffBits(6, BIT_TYPE_BASE_ID);
            monitorBitFrame->looseArbitration(lostArbBit);

            secDriverBitFrame->turnReceivedFrame();

            driverBitFrame->appendBitFrame(secDriverBitFrame);
            monitorBitFrame->appendBitFrame(secMonitorBitFrame);

            driverBitFrame->print(true);
            monitorBitFrame->print(true);

            // Push frames to Lower tester, insert to DUT, run and check!
            pushFramesToLowerTester(*driverBitFrame, *monitorBitFrame);
            startDriverAndMonitor();

            testMessage("Sending frame via DUT!");
            this->dutIfc->sendFrame(dutFrame);
            testMessage("Sent frame via DUT!");

            waitForDriverAndMonitor();
            checkLowerTesterResult();

            deleteCommonObjects();
            delete secDriverBitFrame;
            delete secMonitorBitFrame;
        }

        int run()
        {
            // Run Base test to setup TB
            TestBase::run();
            testMessage("Test %s : Run Entered", testName);

            // Start monitoring when DUT starts transmitting, have no delay then!
            canAgentMonitorSetTrigger(CAN_AGENT_MONITOR_TRIGGER_TX_FALLING);
            canAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));

            // Configure driver to wait for monitor so that LT sends ACK in right moment.
            canAgentSetWaitForMonitor(true);

            // Enable TX/RX feedback so that DUT will see its own transmitted frame!
            canAgentConfigureTxToRxFeedback(true);

            // Run test
            run_can_2_0();
            if (canVersion == CAN_FD_ENABLED_VERSION)
                run_can_fd();

            testControllerAgentEndTest(testResult);
            testMessage("Test %s : Run Exiting", testName);
            return testResult;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};