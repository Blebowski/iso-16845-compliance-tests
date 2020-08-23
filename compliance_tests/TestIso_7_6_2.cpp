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
 * @test ISO16845 7.6.2
 * 
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        a bit error during the transmission of an overload flag.
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
 *      #1 corrupting the first bit of the overload flag;
 *      #2 corrupting the third bit of the overload flag;
 *      #3 corrupting the sixth bit of the overload flag.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  The LT corrupts one of the dominant bits of the overload flag according to
 *  elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 on the corrupted bit.
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

class TestIso_7_6_2 : public test_lib::TestBase
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
                        bitToCorrupt = 1;
                    else if (j == 1)
                        bitToCorrupt = 3;
                    else
                        bitToCorrupt = 6;

                    TestMessage("Forcing Overload flag bit %d to recessive", bitToCorrupt);

                    // Convert to Bit frames
                    driver_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitor_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    /**
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Force last bit of EOF to Dominant!
                     *   3. Insert Overload frame from first bit of Intermission.
                     *   4. Flip n-th bit of Overload flag to RECESSIVE!
                     *   5. Insert Active Error frame to both monitored and driven
                     *      frame!
                     */
                    monitor_bit_frame->TurnReceivedFrame();
                    driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                    driver_bit_frame->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

                    monitor_bit_frame->InsertOverloadFrame(
                        monitor_bit_frame->GetBitOf(0, BitType::Intermission));
                    driver_bit_frame->InsertOverloadFrame(
                        driver_bit_frame->GetBitOf(0, BitType::Intermission));

                    // Force n-th bit of Overload flag on can_rx (driver) to RECESSIVE
                    Bit *bit = driver_bit_frame->GetBitOf(bitToCorrupt - 1, BitType::OverloadFlag);
                    int bitIndex = driver_bit_frame->GetBitIndex(bit);
                    bit->bit_value_ = BitValue::Recessive;

                    // Insert Error flag from one bit further, both driver and monitor!
                    driver_bit_frame->InsertActiveErrorFrame(bitIndex + 1);
                    monitor_bit_frame->InsertActiveErrorFrame(bitIndex + 1);

                    driver_bit_frame->Print(true);
                    monitor_bit_frame->Print(true);

                    // Push frames to Lower tester, run and check!
                    PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    ////////////////////////////////////////////////////////////
                    // Receiver will make received frame valid on 6th bit of EOF!
                    // Therefore at point where Error occurs, frame was already
                    // received OK and should be readable!
                    ////////////////////////////////////////////////////////////
                    Frame readFrame = this->dut_ifc->ReadFrame();
                    if (CompareFrames(*golden_frame, readFrame) == false)
                    {
                        test_result = false;
                        TestControllerAgentEndTest(test_result);
                    }

                    // Check that REC has incremented by 8
                    recNew = dut_ifc->GetRec();

                    // For first iteration we start from 0 so there will be no
                    // decrement on sucessfull reception! For further increments
                    // there will be alway decrement by 1 and increment by 8.
                    int recIncrement;
                    if (i == 0 && j == 0)
                        recIncrement = 8;
                    else
                        recIncrement = 7;

                    if (recNew != (rec + recIncrement))
                    {
                        TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                     rec + recIncrement, recNew);
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