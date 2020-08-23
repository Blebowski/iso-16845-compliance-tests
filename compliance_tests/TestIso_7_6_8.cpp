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
 * @test ISO16845 7.6.8
 * 
 * @brief This test verifies that the IUT increases its REC by 1 when detecting
 *        a form error on the EOF field during reception of a data frame.
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
 *      #1 corrupting the second bit of the EOF;
 *      #2 corrupting the third bit of the EOF;
 *      #3 corrupting the fifth bit of the EOF.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  The LT sends a frame with a stuff error in it and force 1 bit of error flag
 *  to recessive.
 *  This initializes the REC counter to 1 + 8 REC = 9.
 * 
 * Execution:
 *  The LT sends a frame with the EOF modified according to elementary test
 *  cases.
 *
 * Response:
 *  The REC value shall be decreased by 1 because the frame is error free until
 *  ACK.
 *  The REC value shall be increased by 1 on the replaced bit of the EOF.
 *  The REC value shall be unchanged as previous initialized while set up.
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

class TestIso_7_6_8 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            /*****************************************************************
             * Setup part (to get REC to 9)
             ****************************************************************/
            TestMessage("Setup part of test to get REC to 9!");

            // CAN 2.0 / CAN FD, DLC = 1, DATA Frame, Data byte = 0x01
            // randomize Identifier 
            FrameFlags frameFlagsSetup = FrameFlags(FrameType::Can2_0, RtrFlag::DataFrame);
            uint8_t dataByteSetup = 0x80;
            golden_frame = new Frame(frameFlagsSetup, 1, &dataByteSetup);
            golden_frame->Randomize();
            TestBigMessage("Setup frame:");
            golden_frame->Print();

            // Convert to Bit frames
            driver_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);
            monitor_bit_frame = new BitFrame(*golden_frame,
                &this->nominal_bit_timing, &this->data_bit_timing);

            /**
             * Modify setup frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
             *      This will cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Flip first bit of active error frame.
             *   5. Insert Error frame from first bit of Error frame further!
             */
            monitor_bit_frame->TurnReceivedFrame();
            driver_bit_frame->GetBitOf(6, BitType::Data)->FlipBitValue();

            monitor_bit_frame->InsertActiveErrorFrame(
                monitor_bit_frame->GetBitOf(7, BitType::Data));
            driver_bit_frame->InsertActiveErrorFrame(
                driver_bit_frame->GetBitOf(7, BitType::Data));

            // Force 1st bit of Active Error flag on can_rx (driver) to RECESSIVE
            Bit *bit = driver_bit_frame->GetBitOf(0, BitType::ActiveErrorFlag);
            bit->bit_value_ = BitValue::Recessive;

            monitor_bit_frame->InsertActiveErrorFrame(
                monitor_bit_frame->GetBitOf(1, BitType::ActiveErrorFlag));
            driver_bit_frame->InsertActiveErrorFrame(
                driver_bit_frame->GetBitOf(1, BitType::ActiveErrorFlag));

            // Push frames to Lower tester, run and check!
            PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            int recSetup = dut_ifc->GetRec();
            if (recSetup != 9)
            {
                TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                9, recSetup);
                test_result = false;
                TestControllerAgentEndTest(test_result);
                return test_result;
            }
            DeleteCommonObjects();

            /*****************************************************************
             * Common part of test (i=0) / CAN FD enabled part of test (i=1)
             ****************************************************************/

            int iterCnt;
            int rec;
            int recNew;
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

                for (int j = 0; j < 3; j++)
                {
                    // CAN 2.0 / CAN FD, randomize others
                    FrameFlags frameFlags = FrameFlags(dataRate);
                    golden_frame = new Frame(frameFlags);
                    golden_frame->Randomize();
                    TestBigMessage("Test frame:");
                    golden_frame->Print();

                    // Read REC before scenario
                    rec = dut_ifc->GetRec();

                    int bitToCorrupt;
                    if (j == 0)
                        bitToCorrupt = 2;
                    else if (j == 1)
                        bitToCorrupt = 3;
                    else
                        bitToCorrupt = 5;

                    TestMessage("Forcing EOF bit %d to Dominant",
                                    bitToCorrupt);

                    // Convert to Bit frames
                    driver_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitor_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    /**
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Flip n-th bit of EOF to DOMINANT
                     *   3. Insert expected Active error frame from next bit
                     *      of EOF!
                     */
                    monitor_bit_frame->TurnReceivedFrame();
                    driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    driver_bit_frame->GetBitOf(bitToCorrupt - 1, BitType::Eof)->bit_value_ =
                        BitValue::Dominant;

                    driver_bit_frame->InsertActiveErrorFrame(
                        driver_bit_frame->GetBitOf(bitToCorrupt, BitType::Eof));
                    monitor_bit_frame->InsertActiveErrorFrame(
                        monitor_bit_frame->GetBitOf(bitToCorrupt, BitType::Eof));

                    driver_bit_frame->Print(true);
                    monitor_bit_frame->Print(true);

                    // Push frames to Lower tester, run and check!
                    PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    recNew = dut_ifc->GetRec();

                    // Check that REC was not incremented
                    if (recNew != rec)
                    {
                        TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                        rec, recNew);
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