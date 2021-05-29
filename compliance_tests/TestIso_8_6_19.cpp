/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 28.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.19
 * 
 * @brief This test verifies that an IUT acting as a transmitter does not
 *        increase its TEC when detecting a stuff error during arbitration
 *        when monitoring a dominant bit.
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
 *     #1 The LT forces a recessive stuff bit of arbitration field to a
 *        dominant state.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *  The LT causes the IUT to transmit a frame, where the LT causes an error
 *  scenario to init TEC to 08 h before test started.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value.
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

class TestIso_8_6_19 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            dut_ifc->SetTec(8);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                                                       EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x400);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Flip 7-th identifier bit to dominant. This should be recessive stuff bit and it
             *      should cause stuff error!
             *   3. Insert Active Error frame to monitored frame. Insert Passive Error  frame to
             *      driven frame since TX/RX feedback is enabled.
             *   4. Append Retransmitted frame.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->FlipBitAndCompensate(
                driver_bit_frm->GetBitOf(6, BitType::BaseIdentifier), dut_input_delay);

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::BaseIdentifier);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::BaseIdentifier);

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
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            /* +0 for stuff error during arbitration, -1 for succesfull retransmission */
            CheckTecChange(tec_old, -1);

            return FinishElementaryTest();
        }

};