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
 * @test ISO16845 7.6.3
 * 
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        the eighth consecutive dominant bit following the transmission of its
 *        active error flag and after each sequence of additional 8 consecutive
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
 *  The LT causes the IUT to generate an active error frame in data field.
 *  After the error flag sent by the IUT, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 on each eighth dominant bit
 *  after the error flag.
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

class TestIso_7_6_3 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/

            int iterCnt;
            int rec;
            int recNew;
            FrameType dataRate;
            uint8_t dataByte = 0x80;

            if (dut_can_version == CanVersion::CanFdEnabled)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                if (i == 0)
                {
                    TestMessage("Common part of test!");
                    dataRate = FrameType::Can2_0;
                } else {
                    TestMessage("CAN FD enabled part of test!");
                    dataRate = FrameType::CanFd;
                }
                
                // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
                // randomize Identifier 
                FrameFlags frameFlags = FrameFlags(dataRate, RtrFlag::DataFrame);
                golden_frame = new Frame(frameFlags, 1, &dataByte);
                golden_frame->Randomize();
                TestBigMessage("Test frame:");
                golden_frame->Print();

                // Read REC before scenario
                rec = dut_ifc->GetRec();

                // Convert to Bit frames
                driver_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);
                monitor_bit_frame = new BitFrame(*golden_frame,
                    &this->nominal_bit_timing, &this->data_bit_timing);

                /**
                 * Modify test frames:
                 *   1. Monitor frame as if received.
                 *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
                 *      This will cause stuff error!
                 *   3. Insert Active Error frame from 8-th bit of data frame!
                 *   4. Insert 16 Dominant bits directly after Error frame (from first bit
                 *      of Error Delimiter). These bits shall be driven on can_tx, but 16
                 *      RECESSIVE bits shall be monitored on can_tx.
                 */
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(6, BitType::Data)->FlipBitValue();

                monitor_bit_frame->InsertActiveErrorFrame(
                    monitor_bit_frame->GetBitOf(7, BitType::Data));
                driver_bit_frame->InsertActiveErrorFrame(
                    driver_bit_frame->GetBitOf(7, BitType::Data));

                Bit *errDelim = driver_bit_frame->GetBitOf(0, BitType::ErrorDelimiter);
                int bitIndex = driver_bit_frame->GetBitIndex(errDelim);

                for (int k = 0; k < 16; k++)
                {
                    driver_bit_frame->InsertBit(Bit(BitType::ActiveErrorFlag, BitValue::Dominant,
                        &frameFlags, &nominal_bit_timing, &data_bit_timing), bitIndex);
                    monitor_bit_frame->InsertBit(Bit(BitType::ActiveErrorFlag, BitValue::Recessive,
                        &frameFlags, &nominal_bit_timing, &data_bit_timing), bitIndex);
                }

                driver_bit_frame->Print(true);
                monitor_bit_frame->Print(true);

                // Push frames to Lower tester, run and check!
                PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                // Check no frame is received by DUT
                if (dut_ifc->HasRxFrame())
                {
                    TestMessage("DUT has received frame but should not have!");
                    test_result = false;
                }

                // Check that REC has incremented by 25
                //      +1 for original error frame,
                //      +8 for detecting dominant bit first bit after error flag.
                //      +2*8 for each 8 dominant bits after Error flag!
                //  after each error flag!)
                recNew = dut_ifc->GetRec();
                if (recNew != (rec + 25))
                {
                    TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                    rec + 25, recNew);
                    test_result = false;
                    TestControllerAgentEndTest(test_result);
                    return test_result;
                }

                DeleteCommonObjects();
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};