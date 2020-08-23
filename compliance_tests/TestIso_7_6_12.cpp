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
 * @test ISO16845 7.6.12
 * 
 * @brief This test verifies that a receiver increases its REC by 1 when
 *        detecting a form error on a bit of the error delimiter it is
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
 *      #1 the second bit of the error delimiter is corrupted;
 *      #2 the seventh bit of the error delimiter is corrupted.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  The LT corrupts 1 bit of the error delimiter according to elementary
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

class TestIso_7_6_12 : public test_lib::TestBase
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
            uint8_t dataByte = 0x80;
            FrameType dataRate;

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

                for (int j = 0; j < 2; j++)
                {
                    // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
                    // randomize Identifier 
                    FrameFlags frameFlags = FrameFlags(dataRate, RtrFlag::DataFrame);
                    golden_frame = new Frame(frameFlags, 1, &dataByte);
                    golden_frame->Randomize();
                    TestBigMessage("Test frame:");
                    golden_frame->Print();

                    // Read REC before scenario
                    rec = dut_ifc->GetRec();

                    int bitToCorrupt;
                    if (j == 0)
                        bitToCorrupt = 2;
                    else
                        bitToCorrupt = 7;

                    TestMessage("Forcing Error delimiter bit %d to Dominant",
                                    bitToCorrupt);

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
                     *   4. Flip 2nd or 7th bit of Error delimiter to dominant!
                     *   5. Insert next active error frame from 3-rd or 8-th bit
                     *      of Error delimiter!
                     */
                    monitor_bit_frame->TurnReceivedFrame();
                    driver_bit_frame->GetBitOf(6, BitType::Data)->FlipBitValue();

                    monitor_bit_frame->InsertActiveErrorFrame(
                        monitor_bit_frame->GetBitOf(7, BitType::Data));
                    driver_bit_frame->InsertActiveErrorFrame(
                        driver_bit_frame->GetBitOf(7, BitType::Data));

                    // Force n-th bit of Error Delimiter to dominant!
                    Bit *bit = driver_bit_frame->GetBitOf(bitToCorrupt - 1, BitType::ErrorDelimiter);
                    int bitIndex = driver_bit_frame->GetBitIndex(bit);
                    bit->bit_value_ = BitValue::Dominant;

                    monitor_bit_frame->InsertActiveErrorFrame(
                        monitor_bit_frame->GetBitOf(bitToCorrupt, BitType::ErrorDelimiter));
                    driver_bit_frame->InsertActiveErrorFrame(
                        driver_bit_frame->GetBitOf(bitToCorrupt, BitType::ErrorDelimiter));

                    driver_bit_frame->Print(true);
                    monitor_bit_frame->Print(true);

                    // Push frames to Lower tester, run and check!
                    PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    recNew = dut_ifc->GetRec();

                    // Check that REC was incremented by 2
                    // (1 stuff error in data field, 1 form error in error delimiter!)
                    if (recNew != rec + 2)
                    {
                        TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                        rec + 2, recNew);
                        test_result = false;
                        TestControllerAgentEndTest(test_result);
                        return test_result;
                    }
                    DeleteCommonObjects();
                }
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};