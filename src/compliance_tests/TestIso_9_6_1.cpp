/*****************************************************************************
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
 * @date 14.2.2021
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 9.6.1
 *
 * @brief This test verifies that increasing REC and TEC are independent operations.
 * @version CAN FD enabled
 *
 * Test variables:
 *     Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          REC
 *          TEC
 *          FDF = 0
 *
 *     CAN FD Enabled:
 *          REC
 *          TEC
 *          FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to increase its REC up to 127. Then, LT causes the
 *  IUT to increase its TEC up to 128. Then, the LT sends a frame containing
 *  a stuff error in data field.
 *
 * Response:
 *  Each increment of the TEC shall be responded by an active error flag.
 *  The IUT responds to the stuff error with a passive error flag.
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

class TestIso_9_6_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);

            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorActive);
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0xAA, &data_byte);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, 0x1, 0xAA, &data_byte);
            golden_frm->Print();

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            // Separate frame is needed for CAN FD enabled variant. This frame is already with
            // IUT being Error passive, so we need frame/frame_flags with ESI error passive!
            driver_bit_frm_3 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_3 = ConvertBitFrame(*golden_frm_2);

            driver_bit_frm_4 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_4 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip 7-th bit of data field. This should bit stuff bit. Do this in both frames.
             *   2. Insert Active Error frame to Monitored frame from next bit. Insert Active
             *      Error frame to driven frame (TX/RX feedback disabled). Do this in both frames.
             *   3. Append second frame behind the first one 15 times. This should account for
             *      15 retransmissions. After this, the TEC should have just increased to 128 and
             *      IUT should have just become error passive.
             *   4. Append Suspend Transmission since IUT just became error passive!
             *   5. In third frame, again flip 7-th bit of data field to cause stuff error.
             *      Now insert passive error frame since IUT should have just became error passive!
             *   6. Append one more frame with ACK bit set (so that IUT will not retransmitt
             *      indefinitely).
             *************************************************************************************/
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();
            driver_bit_frm_2->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            driver_bit_frm_2->InsertActiveErrorFrame(7, BitType::Data);
            monitor_bit_frm_2->InsertActiveErrorFrame(7, BitType::Data);

            for (int i = 0; i < 15; i++)
            {
                driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());
            }

            driver_bit_frm->AppendSuspendTransmission();
            monitor_bit_frm->AppendSuspendTransmission();

            driver_bit_frm_3->GetBitOf(6, BitType::Data)->FlipBitValue();
            driver_bit_frm_3->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm_3->InsertPassiveErrorFrame(7, BitType::Data);

            driver_bit_frm->AppendBitFrame(driver_bit_frm_3.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_3.get());

            driver_bit_frm->AppendSuspendTransmission();
            monitor_bit_frm->AppendSuspendTransmission();

            driver_bit_frm_4->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
            driver_bit_frm->AppendBitFrame(driver_bit_frm_4.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_4.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(127);
            dut_ifc->SetTec(0);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
};