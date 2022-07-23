/****************************************************************************** 
 * 
 * ISO16845 Compliance tests 
 * Copyright (C) 2021-present Ondrej Ille
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 * 
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 30.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.5.7
 * 
 * @brief The purpose of this test is to verify that an IUT changes its state
 *        from active to passive.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Error at error frame, FDF = 0
 * 
 *  CAN FD Enabled
 *      Error at error frame, FDF = 1
 * 
 * Elementary test cases:
 *  There is one test to perform.
 *      #1 Bit error up to REC passive limit by sending 17 recessive bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  The LT corrupts the following active error flag according to elementary
 *  test cases. After this sequence, the IUT shall be error passive and sending
 *  a passive error flag.
 *  The LT send a valid frame 6 + 8 + 3 bit after dominant part of previous
 *  error sequence.
 * 
 * Response:
 *  The IUT shall generate a passive error flag starting at the bit position
 *  following the last recessive bit sent by the LT.
 *  The IUT shall acknowledge the following test frame.
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
using namespace test_lib;

class TestIso_7_5_7 : public test_lib::TestBase
{
    public:
        
        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                            RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &error_data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff bit
             *      therefore causing error.
             *   3. Remove all bits from next bit on.
             *   4. Append 17 recessive bits to driven frame and 17 dominant bits to monitored
             *      frame. This corresponds to retransmissions of active error flag by IUT.
             *   5. Append Passive Error frame to monitored frame and also to driven frame (this
             *      also includes Intermission)
             *   6. Append next frame as if received by IUT.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->RemoveBitsFrom(7, BitType::Data);
            monitor_bit_frm->RemoveBitsFrom(7, BitType::Data);

            /* 
             * We need to insert 18 since following insertion of passive error frame over
             * writes bit from which error frame starts!
             */
            for (int i = 0; i < 18; i++)
            {
                driver_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Dominant);
            }

            int last_bit = driver_bit_frm->GetBitCount();
            driver_bit_frm->InsertPassiveErrorFrame(last_bit - 1);
            monitor_bit_frm->InsertPassiveErrorFrame(last_bit - 1);

            monitor_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(0); /* Must be reset before every elementary test */
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);
            CheckNoRxFrame(); /* Only one frame should be received! */
            
            FreeTestObjects();
            return FinishElementaryTest();
        }
};