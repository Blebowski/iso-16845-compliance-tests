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
 * @date 14.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.20
 * 
 * @brief This test verifies that the IUT does not change the value of its TEC
 *        when receiving a frame successfully.
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
 *     #1 This frame is disturbed by an error to increase TEC.
 * 
 * Setup:
 *  No action is required. The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT sends a frame with higher ID priority to cause the IUT to lose
 *  arbitration.
 *  The LT receives the repeated frame without error.
 *  
 * Response:
 *  The IUT’s TEC value shall be only decreased by 1 for successful
 *  transmission at the end of test execution.
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

class TestIso_8_6_20 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 1;
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            /* Preset TEC so that there is what to decrement */
            dut_ifc->SetTec(8);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* Sent by LT */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                                       EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x50);
            RandomizeAndPrint(golden_frm.get());

            /* Sent by IUT */
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                                         EsiFlag::ErrorActive);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, 0x1, 0x51);
            RandomizeAndPrint(golden_frm_2.get());

            /* Since IUT will loose arbitration, do both driven and monitored frames as the ones
             * from IUT, correct the last bit later
             */
            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* In retransmitted frame, there will be no arbitration lost */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip last bit of base id of monitored frame to recessive since IUT actually
             *      sends ID ending with 1.
             *   2. Loose arbitration in monitored frame on last bit of base id.
             *   3. Append retransmitted frame by IUT.
             *************************************************************************************/
            Bit *last_base_id = monitor_bit_frm->GetBitOfNoStuffBits(10, BitType::BaseIdentifier);
            last_base_id->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->LooseArbitration(last_base_id);

            // Compensate resynchronisation due to IUTs input delay
            last_base_id->GetLastTimeQuantaIterator(BitPhase::Ph2)->Lengthen(dut_input_delay);

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());                    

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm_2.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            CheckTecChange(tec_old, -1);

            return FinishElementaryTest();
        }

};