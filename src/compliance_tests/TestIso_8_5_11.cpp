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
 * @date 3.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.11
 *
 * @brief The purpose of this test is to verify that an IUT which is bus-off
 *        is not permitted to become error active (no longer bus-off) before
 *        128 occurrences of 11 consecutive recessive bits.
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
 *      1) the LT sends recessive bus level for at least 1 408 bit times until
 *         the IUT becomes active again;
 *      2) the LT sends one group of 10 recessive bits, one group of 21 recessive
 *         bits followed by at least 127 groups of 11 recessive bits, each group
 *         separated by 1 dominant bit.
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT ask the IUT to send a frame and sets it in the bus-off state.
 *
 *  The LT sends profiles defined in elementary test cases.
 *
 * Response:
 *  The IUT shall not transmit the frame before the end of the profiles sent by
 *  the LT according to elementary test cases and shall send it before the end
 *  of the TIMEOUT.
 *
 * Note:
 *  Check error counter after bus-off, if applicable.
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

class TestIso_8_5_11 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 2; i++)
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

            /* First frame */
            frame_flags = std::make_unique<FrameFlags>(
                elem_test.frame_type_, IdentifierType::Base, RtrFlag::DataFrame,
                BrsFlag::DontShift, EsiFlag::ErrorPassive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            // In FD enabled variant, the retransmitted frame will be in error active
            // state, so ESI must be different! Other frame flags MUST be the same,
            // otherwise we may get different frames!
            if (test_variant == TestVariant::CanFdEnabled) {
                frame_flags_2 = std::make_unique<FrameFlags>(
                    elem_test.frame_type_, IdentifierType::Base, RtrFlag::DataFrame,
                    BrsFlag::DontShift, EsiFlag::ErrorActive);

                golden_frm->frame_flags_ = *frame_flags_2;
            }
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);


            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frames as received. Force ACK Delimiter low. This will cause form
             *      error at transmitter and unit to become bus-off.
             *   2. Insert Passive Error frame from next bit on to both driven and monitored
             *      frames.
             *   3. Append test sequences as given by elementary test.
             *
             *   Note: This does not check that frame will be retransmitted before timeout!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::AckDelimiter)->bit_value_ = BitValue::Dominant;

            Bit *eof_bit = driver_bit_frm->GetBitOf(0, BitType::Eof);
            int eof_start = driver_bit_frm->GetBitIndex(eof_bit);

            driver_bit_frm->InsertPassiveErrorFrame(eof_start);
            monitor_bit_frm->InsertPassiveErrorFrame(eof_start);

            int interm_index = driver_bit_frm->GetBitIndex(
                                driver_bit_frm->GetBitOf(0, BitType::Intermission));
            driver_bit_frm->RemoveBitsFrom(interm_index);
            monitor_bit_frm->RemoveBitsFrom(interm_index);

            if (elem_test.index_ == 1)
            {
                for (int i = 0; i < 1408; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }

                // IUT specific compensation to exactly match after what time IUT
                // joins the bus. This checks exactly for given IUT, not the "minimum"
                // time as stated in the test! This is more strict, however it would
                // need to be adjusted for other implementation!
                // TODO: Genealize for other implementations than CTU CAN FD!
                for (int i = 0; i < 14; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                Bit *last = monitor_bit_frm->GetBit(monitor_bit_frm->GetBitCount() - 1);
                last->GetTimeQuanta(0)->Lengthen(dut_input_delay);

            } else {

                for (int i = 0; i < 10; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                driver_bit_frm->AppendBit(BitType::Idle, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);

                for (int i = 0; i < 21; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                driver_bit_frm->AppendBit(BitType::Idle, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);

                for (int i = 0; i < 127; i++)
                {
                    for (int j = 0; j < 11; j++)
                    {
                        driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                        monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    }
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Dominant);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);

                // IUT specific compensation to exactly match after what time IUT
                // joins the bus. This checks exactly for given IUT, not the "minimum"
                // time as stated in the test! This is more strict, however it would
                // need to be adjusted for other implementation!
                // TODO: Genealize for other implementations than CTU CAN FD!
                for (int i = 0; i < 11; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                Bit *last = monitor_bit_frm->GetBit(monitor_bit_frm->GetBitCount() - 1);
                last->GetTimeQuanta(0)->Lengthen(dut_input_delay);
            }

            // Re-transmitted frame after reintegration!
            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            dut_ifc->SetTec(255); /* just before bus-off */
            dut_ifc->SendReintegrationRequest(); /* Request in advance, DUT will hold it */
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            /* Must restart DUT for next iteration since it is bus off! */
            dut_ifc->Disable();
            dut_ifc->Reset();
            dut_ifc->ConfigureBitTiming(nominal_bit_timing, data_bit_timing);
            dut_ifc->Enable();

            TestMessage("Waiting till DUT is error active!");
            while (dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(2000);

            return FinishElementaryTest();
        }

};