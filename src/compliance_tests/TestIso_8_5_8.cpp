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
 * @test ISO16845 8.5.8
 * 
 * @brief The purpose of this test is to verify that a passive state IUT, after
 *        losing arbitration, repeats the frame without inserting any suspend
 *        transmission.
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
 *      #1 The LT causes the IUT to lose arbitration by sending a frame of
 *         higher priority.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test cases.
 * 
 * Response:
 *  The LT verifies that the IUT re-transmits its frame (1 + 7 + 3) bit times
 *  after acknowledging the received frame.
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

class TestIso_8_5_8 : public test_lib::TestBase
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
            /* To avoid stuff bits causing mismatches betwen frame lengths */
            uint8_t data_byte = 0xAA;

            /* ESI needed for CAN FD variant */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x44A, &data_byte);

            /* This frame should win arbitration */
            golden_frm_2 = std::make_unique<Frame>(*frame_flags, 0x1, 0x24A, &data_byte);
            RandomizeAndPrint(golden_frm.get());
            RandomizeAndPrint(golden_frm_2.get());

            /* This will be frame beating IUT with lower ID */
            driver_bit_frm = ConvertBitFrame(*golden_frm_2);

            /* This is frame sent by IUT (ID= 0x200)*/ 
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* This is retransmitted frame by IUT */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Loose arbitration on monitored frame on first bit (0x245 vs 0x445 IDs).
             *   2. Compensate IUTs input delay, since next edge applied by LT will be perceived
             *      as shifted by IUT, due to its input delay.
             *   3. Do iteration specific compensation due to different number of stuff bits (since
             *      there are different IDs and CRCs):
             *        A. Common variant - remove one bit
             *   4. Append the same frame, retransmitted.
             *************************************************************************************/
            monitor_bit_frm->LooseArbitration(0, BitType::BaseIdentifier);
            monitor_bit_frm->GetBitOf(0, BitType::BaseIdentifier)
                ->GetFirstTimeQuantaIterator(BitPhase::Sync)->Lengthen(dut_input_delay);

            if (test_variant == TestVariant::Common)
                monitor_bit_frm->RemoveBit(monitor_bit_frm->GetBitOf(0, BitType::Data));

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

            return FinishElementaryTest();
        }

};