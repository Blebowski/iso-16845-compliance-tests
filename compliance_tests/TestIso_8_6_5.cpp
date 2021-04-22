/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.5
 * 
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting 8 consecutive dominant bits following the
 *        transmission of its overload flag and after each sequence of
 *        additional 8 consecutive dominant bits.
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
 *      #1 Dominant bits after overload flag: 23 bits
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame after a data frame.
 *  After the overload flag sent by the IUT, the LT sends a sequence dominant
 *  bits according to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be increased by 8 on each eighth dominant bit
 *  after the overload flag.
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

class TestIso_8_6_5 : public test_lib::TestBase
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
             *   2. Force first bit of intermission low (overload condition)
             *   3. Insert 23 dominant bits after the overload flag!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(0, BitType::Intermission)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

            for (int i = 0; i < 23; i++)
            {
                int bit_index = driver_bit_frm->GetBitIndex(
                                    driver_bit_frm->GetBitOf(5, BitType::OverloadFlag));
                driver_bit_frm->InsertBit(BitType::OverloadFlag, BitValue::Dominant, bit_index + 1);
                monitor_bit_frm->InsertBit(BitType::OverloadDelimiter, BitValue::Recessive, bit_index + 1);
            }

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

            /* 23 bits = 2 * 8 bits -> Increment + 16. -1 decrement for succesfull transmission.
             * This is skipped on first elementary test since TEC is 0.*/
            if (test_variant == TestVariant::Common && elem_test.index == 1)
                CheckTecChange(tec_old, 16);
            else
                CheckTecChange(tec_old, 15);

            return FinishElementaryTest();
        }

};