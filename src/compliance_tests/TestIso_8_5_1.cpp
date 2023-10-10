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
 * @date 29.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.5.1
 * 
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter does not detect any error when detecting an active
 *        error flag during its own passive error flag.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      FDF = 1
 * 
 * Elementary test cases:
 *   The LT replaces one of the 8 recessive bits of the error delimiter by a
 *   dominant bit.
 *      #1 superposing the passive error flag by an active error flag starting
 *         at the first bit;
 *      #2 superposing the passive error flag by an active error flag starting
 *         at the third bit;
 *      #3 superposing the passive error flag by an active error flag starting
 *         at the sixth bit.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to send a passive error flag in data field.
 *  During the passive error flag sent by the IUT, the LT sends an active error
 *  flag in date field according to elementary test cases.
 *  At the end of the error flag, the LT waits for (8 + 3) bit time before
 *  sending a frame.
 * 
 * Response:
 *  The IUT shall acknowledge the last frame transmitted by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "pli_lib.h"

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

class TestIso_8_5_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 3;
            for (int i = 0; i < num_elem_tests; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            // Since there is one frame received in between, IUT will resynchronize and
            // mismatches in data bit rate can occur. Dont shift bit-rate due to this
            // reason.
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);
        
            golden_frm_2 = std::make_unique<Frame>(*frame_flags, 1, &data_byte);
            RandomizeAndPrint(golden_frm_2.get());

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force 7-th data bit to dominant (should be recessive stuff bit), this creates
             *      stuff error.
             *   3. Insert Passive Error frame to both driver and monitor from next bit
             *   4. Insert Active Error frame to 1/3/6th bit of Passive Error flag on driven frame.
             *      Insert Passive Error flag to monitored frame. On monitor this emulates waiting
             *      by DUT to monitor recessive bit after error flag.
             *   5. Append next frame right behind (8 bits - delimiter, 3 bits - intermission
             *      create exactly desired separation.).
             *   6. Append the first frame once again, since the IUT will retransmitt this (due to
             *      error in first frame)! It did not retransmitt it during second frame because it
             *      turned receiver due to suspend!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            int bit_index_to_corrupt;
            if (elem_test.index_ == 1)
                bit_index_to_corrupt = 0;
            else if (elem_test.index_ == 2)
                bit_index_to_corrupt = 2;
            else
                bit_index_to_corrupt = 5;

            Bit *bit_to_corrupt = driver_bit_frm->GetBitOf(bit_index_to_corrupt,
                                    BitType::PassiveErrorFlag);
            int bit_index = driver_bit_frm->GetBitIndex(bit_to_corrupt);
            TestMessage("Inserting Active Error flag to Passive Error flag bit %d to dominant",
                        bit_index_to_corrupt + 1);
            
            driver_bit_frm->InsertActiveErrorFrame(bit_index);
            monitor_bit_frm->InsertPassiveErrorFrame(bit_index);

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm_2->TurnReceivedFrame();
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            // Here we need compensation of the second frame which is transmitted by LT
            // from SOF on. This is overally second SOF bit (first is in the first frame)!
            // Since LT starts transmitting the frame now, it will take input delay since
            // edge is seen by IUT, therefore IUT will execute positive resynchronization!
            monitor_bit_frm->GetBitOf(1, BitType::Sof)
                ->GetLastTimeQuantaIterator(BitPhase::Sync)->Lengthen(dut_input_delay);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);
            driver_bit_frm_2->TurnReceivedFrame();

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /***************************************************************************** 
             * Execute test
             *****************************************************************************/
            dut_ifc->SetTec(160);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm_2);

            return FinishElementaryTest();
        }

};