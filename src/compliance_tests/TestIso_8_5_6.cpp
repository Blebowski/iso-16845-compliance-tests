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
 * @test ISO16845 8.5.6
 * 
 * @brief The purpose of this test is to verify that a passive state IUT being
 *        transmitter does not transmit any data frame before the end of the
 *        suspend transmission following an overload frame.
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
 *   There is one elementarty test to perform:
 *      #1 After overload flag, the LT forces the bus to recessive for overload
 *         delimiter + intermission + suspend transmission time.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit two frames.
 *  The LT lets the IUT transmit the first frame and causes the IUT to generate
 *  an overload frame according to elementary test cases.
 * 
 * Response:
 *  After the intermission following the overload frame, the LT verifies that
 *  the IUT wait 8 more bits before transmitting the second frame.
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

class TestIso_8_5_6 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 1;
            AddElemTest(TestVariant::Common, ElementaryTest(1 , FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* ESI needed for CAN FD variant */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force first bit of Intermission in driven frame to Dominant (Overload
             *      condition).
             *   3. Insert Overload frame to monitored frame. Insert Passive Error frame to driven
             *      frame (same length as overload, only recessiv bits) from  next bit on.
             *   4. Append suspend transmission field to both driven and monitored frames.
             *   5. Append next frame.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            Bit *first_interm_bit = driver_bit_frm->GetBitOf(0, BitType::Intermission);
            driver_bit_frm->FlipBitAndCompensate(first_interm_bit, dut_input_delay);

            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
            driver_bit_frm->InsertPassiveErrorFrame(1, BitType::Intermission);

            driver_bit_frm->AppendSuspendTransmission();
            monitor_bit_frm->AppendSuspendTransmission();

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
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }

};