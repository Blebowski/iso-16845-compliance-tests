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
 * @date 12.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.5
 *
 * @brief The purpose of this test is to verify the point of time at which a
 *        message transmitted by the IUT is taken to be valid.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled - FDF = 0
 *      CAN FD enabled - FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 * 
 *      #1 On the first bit of the intermission field of the frame sent by the
 *         IUT, the LT forces the bit value to dominant.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a data frame. The LT causes the IUT to
 *  generate an overload frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall not restart any frame after the overload frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_8_1_5 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
            
            /* Basic setup for tests where IUT transmits */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received (insert ACK).
             *   2. Force first bit of driven frame to dominant.
             *   3. Insert overload flag from 2nd bit of intermission further!
             *   4. Insert 15 recessive bits at the end of Overload delimiter.
             *      This checks that DUT does not retransmitt the frame!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->CompensateEdgeForInputDelay(
                driver_bit_frm->GetBitOf(0, BitType::Ack), dut_input_delay);

            Bit *first_interm_bit = driver_bit_frm->GetBitOf(0, BitType::Intermission);
            driver_bit_frm->FlipBitAndCompensate(first_interm_bit, dut_input_delay);

            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

            Bit *overload_end_bit = monitor_bit_frm->GetBitOf(6, BitType::OverloadDelimiter);
            int bit_index = monitor_bit_frm->GetBitIndex(overload_end_bit);
            for (int i = 0; i < 15; i++)
                monitor_bit_frm->InsertBit(BitType::OverloadDelimiter, BitValue::Recessive,
                                            bit_index);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            this->dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
    
};