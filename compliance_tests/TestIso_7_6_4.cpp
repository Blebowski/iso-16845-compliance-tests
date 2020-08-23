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

                // CAN 2.0 / CAN FD, randomize others
                FrameFlags frameFlags = FrameFlags(dataRate);
                golden_frame = new Frame(frameFlags);
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
                 *   2. Force last bit of EOF to Dominant!
                 *   3. Insert Overload frame from first bit of Intermission.
                 *   4. Insert 16 Dominant bits directly after Overload frame
                 *      (from first bit of Overload Delimiter). These bits
                 *      shall be driven on can_tx, but 16 RECESSIVE bits shall
                 *      be monitored on can_tx. 
                 */
                monitor_bit_frame->TurnReceivedFrame();
                driver_bit_frame->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                driver_bit_frame->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

                monitor_bit_frame->InsertOverloadFrame(
                    monitor_bit_frame->GetBitOf(0, BitType::Intermission));
                driver_bit_frame->InsertOverloadFrame(
                    driver_bit_frame->GetBitOf(0, BitType::Intermission));

                Bit *ovrDelim = driver_bit_frame->GetBitOf(0, BitType::OverloadDelimiter);
                int bitIndex = driver_bit_frame->GetBitIndex(ovrDelim);

                for (int k = 0; k < 16; k++)
                {
                    driver_bit_frame->InsertBit(Bit(BitType::OverloadFlag, BitValue::Dominant,
                        &frameFlags, &nominal_bit_timing, &data_bit_timing), bitIndex);
                    monitor_bit_frame->InsertBit(Bit(BitType::OverloadFlag, BitValue::Recessive,
                        &frameFlags, &nominal_bit_timing, &data_bit_timing), bitIndex);
                }

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

                recNew = dut_ifc->GetRec();

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
                    TestMessage("DUT REC not as expected. Expected %d, Real %d",
                                    rec + recIncrement, recNew);
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