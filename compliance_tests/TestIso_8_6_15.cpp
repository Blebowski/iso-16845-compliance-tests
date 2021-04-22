/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.15
 * 
 * @brief This test verifies that an IUT acting as a transmitter does not
 *        change the value of its TEC when detecting a form error on the last
 *        bit of the overload delimiter it is transmitting.
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
 *     #1 LT sends 1 dominant bit.
 * 
 * Setup:
 *  The LT forces the IUT to increase its TEC.
 *  The LT causes the IUT to transmit a frame, where the LT causes an error
 *  scenario to set TEC to 08 h before the test started.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame after this data
 *  frame.
 *  At the last bit of the overload delimiter, the LT sends dominant bit
 *  according to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value decreased by 1.
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

class TestIso_8_6_15 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentConfigureTxToRxFeedback(true);
            CanAgentSetWaitForMonitor(true);

            dut_ifc->SetTec(8);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force first bit of Interframe to dominant (overload flag)
             *   3. Insert Expected overload frame from second bit of intermission.
             *   4. Force last bit of overload delimiter (8-th) to dominant.
             *   5. Insert next expected overload frame!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(0, BitType::Intermission)->FlipBitValue();

            driver_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

            driver_bit_frm->GetBitOf(7, BitType::OverloadDelimiter)->FlipBitValue();

            /* Next bit is 2nd intermission bit overally! */
            driver_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

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
            CheckTecChange(tec_old, -1);

            return FinishElementaryTest();
        }

};