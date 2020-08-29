/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 29.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.5.4
 * 
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter is able to receive a frame during the suspend
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
 *   Elementary tests to perform:
 *      #1 the received frame starts on the first bit of the suspend transmission;
 *      #2 the received frame starts on the fourth bit of the suspend transmission;
 *      #3 the received frame starts on the eighth bit of the suspend transmission.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  At the end of the EOF and intermission fields, the LT sends a frame according to
 *  elementary test-cases.
 * 
 * Response:
 *  The IUT shall acknowledge the last frame transmitted by the LT.
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

class TestIso_8_5_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 3;
            for (int i = 0; i < num_elem_tests; i++)
            {
                elem_tests[0].push_back(ElementaryTest(i + 1 , FrameType::Can2_0));
                elem_tests[1].push_back(ElementaryTest(i + 1, FrameType::CanFd));
            }

            dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        int Run()
        {
            SetupTestEnvironment();

            for (int test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(
                                    elem_tests[test_variant][elem_test.index].frame_type,
                                    EsiFlag::ErrorActive); /* ESI needed for CAN FD variant */
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    frame_flags_2 = std::make_unique<FrameFlags>();
                    golden_frm_2 = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm_2.get());

                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Turn driven frame as if received.
                     *   2. Insert suspend field to both driven and monitored frames (it is not
                     *      inserted by default frame construction). Insert only necessary portion
                     *      of field to emulate start of next frame in the middle of it!
                     *   3. Append next frame to driven frame. Append next frame as if received to
                     *      monitored frame.
                     *****************************************************************************/
                    driver_bit_frm->TurnReceivedFrame();

                    int num_suspend_bits;
                    switch (elem_test.index)
                    {
                        case 1:
                            num_suspend_bits = 0;
                            break;
                        case 2:
                            num_suspend_bits = 3;
                            break;
                        case 3:
                            num_suspend_bits = 7;
                            break;
                        default:
                            break;
                    }

                    for (int i = 0; i < num_suspend_bits; i++)
                    {
                        driver_bit_frm->AppendBit(
                            Bit(BitType::Suspend, BitValue::Recessive, frame_flags.get(),
                            &nominal_bit_timing, &data_bit_timing, StuffBitType::NoStuffBit));
                        monitor_bit_frm->AppendBit(
                            Bit(BitType::Suspend, BitValue::Recessive, frame_flags.get(),
                            &nominal_bit_timing, &data_bit_timing, StuffBitType::NoStuffBit));
                    }

                    driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                    monitor_bit_frm_2->TurnReceivedFrame();
                    monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    StartDriverAndMonitor();
                    dut_ifc->SendFrame(golden_frm.get());
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();

                    CheckRxFrame(*golden_frm_2);
                }
            }

            return (int)FinishTest();
        }
};