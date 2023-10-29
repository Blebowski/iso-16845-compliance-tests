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
 * @test ISO16845 8.5.3
 *
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter does not detect any error when detecting dominant
 *        bits during the 7 first bit of the error delimiter.
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
 *   Elementary tests to perform:
 *      #1 transmitting 1 consecutive dominant bits;
 *      #2 transmitting 4 consecutive dominant bits;
 *      #3 transmitting 7 consecutive dominant bits.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a data frame. Then, the LT causes the
 *  IUT to send a passive error flag in data field.
 *  At the end of error flag, the LT continues transmitting dominant bits
 *  according to elementary test cases.
 *  At this step, the LT waits for (8 + 3) bit time before sending a frame.
 *
 * Response:
 *  The IUT shall acknowledge the frame transmitted by the LT.
 *  The IUT shall restart the transmission of the corrupted frame (1 + 7 + 3)
 *  bit times after its ACK bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 3;
            for (int i = 0; i < num_elem_tests; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1 , FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            // Since there is one frame received in between first and third frame,
            // IUT will resynchronize and mismatches in data bit rate can occur. Dont shift
            // bit-rate due to this reason. Alternative is to demand BRP=BRP_FD
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            golden_frm_2 = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm_2.get());

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force 7-th data bit to dominant (should be recessive stuff bit), this creates
             *      stuff error.
             *   3. Insert Passive Error frame to monitored frame and driven frame from next bit.
             *   4. Insert 1,4,7 Dominant bits to driven frame at start of Error delimiter. Insert
             *      the same amount of recessive bits to monitored frame.
             *   5. Insert second frame after the first one. This is exactly after 8 + 3 (Error
             *      delimiter + intermission) to be transmitted by LT.
             *   6. After second frame, append the same frame again and check IUT retransmitts it!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            int bits_to_insert = 0;
            switch (elem_test.index_)
            {
                case 1:
                    bits_to_insert = 1;
                    break;
                case 2:
                    bits_to_insert = 4;
                    break;
                case 3:
                    bits_to_insert = 7;
                    break;
                default:
                    break;
            }

            for (int i = 0; i < bits_to_insert; i++)
            {
                Bit *err_delim_bit = driver_bit_frm->GetBitOf(0, BitType::ErrorDelimiter);
                int err_delim_index = driver_bit_frm->GetBitIndex(err_delim_bit);

                driver_bit_frm->InsertBit(BitType::ActiveErrorFlag, BitValue::Dominant,
                                          err_delim_index);
                monitor_bit_frm->InsertBit(BitType::PassiveErrorFlag, BitValue::Recessive,
                                           err_delim_index);

                /* First dominant bit inserted will cause re-synchronisation due to input
                 * delay. We need to compensate it by lenghtening monitoried sequence (what
                 * IUTs perception of frame is).
                 */
                if (i == 0)
                {
                    // First bit inserted is 7-th passive error flag bit overally.
                    Bit *comp_bit = monitor_bit_frm->GetBitOf(6, BitType::PassiveErrorFlag);
                    comp_bit->GetLastTimeQuantaIterator(BitPhase::Ph2)->Lengthen(dut_input_delay);
                }
            }

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm_2->TurnReceivedFrame();
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            /* Append the original frame, retransmitted by DUT after 2nd frame! */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);
            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm_2);

            return FinishElementaryTest();
        }

};