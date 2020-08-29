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
 * @test ISO16845 8.5.1
 * 
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter does not detect any error when detecting an active
 *        error flag during its own passive error flag.
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
 *   The LT replaces one of the 8 recessive bits of the error delimiter by a
 *   dominant bit.
 *      #1 superposing the passive error flag by an active error flag starting
 *         at the first bit;
 *      #2 superposing the passive error flag by an active error flag starting
 *         at the third bit;
 *      #3 superposing the passive error flag by an active error flag starting
 *         at the sixth bit.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to send a passive error flag in data field.
 *  During the passive error flag sent by the IUT, the LT sends an active error
 *  flag in date field according to elementary test cases.
 *  At the end of the error flag, the LT waits for (8 + 3) bit time before
 *  sending a frame.
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

class TestIso_8_5_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 3;
            for (int i = 0; i < num_elem_tests; i++)
            {
                elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::Can2_0));
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
            uint8_t data_byte = 0x80;

            for (int test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(
                        elem_tests[test_variant][elem_test.index].frame_type, RtrFlag::DataFrame);
                    golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte);
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
                     *   2. Force 7-th data bit to dominant (should be recessive stuff bit), this
                     *      creates stuff error.
                     *   3. Insert Passive Error frame to both driver and monitor from next bit
                     *   4. Insert Active Error frame to 1/3/6th bit of Passive Error flag on
                     *      driven frame. Insert Passive Error flag to monitored frame. On monitor
                     *      this emulates waiting by DUT to monitor recessive bit after error flag.
                     *   5. Append next frame right behind (8 bits - delimiter, 3 bits - intermi-
                     *      ssion create exactly desired separation.).
                     *   6. Append the first frame once again, since the IUT will retransmitt this
                     *      (due to error in first frame)! It did not retransmitt it during second
                     *      frame because it turned receiver due to suspend!
                     *****************************************************************************/
                    driver_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    monitor_bit_frm->InsertPassiveErrorFrame(
                        monitor_bit_frm->GetBitOf(7, BitType::Data));
                    driver_bit_frm->InsertPassiveErrorFrame(
                        driver_bit_frm->GetBitOf(7, BitType::Data));

                    int bit_to_corrupt;
                    if (elem_test.index == 1)
                        bit_to_corrupt = 0;
                    else if (elem_test.index == 2)
                        bit_to_corrupt = 2;
                    else
                        bit_to_corrupt = 5;
                    int bit_index = driver_bit_frm->GetBitIndex(driver_bit_frm->GetBitOf(
                                        bit_to_corrupt, BitType::PassiveErrorFlag));
                    TestMessage("Inserting Active Error flag to Passive Error flag bit %d to dominant",
                                bit_to_corrupt + 1);
                    driver_bit_frm->InsertActiveErrorFrame(bit_index);
                    monitor_bit_frm->InsertPassiveErrorFrame(bit_index);

                    driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                    monitor_bit_frm_2->TurnReceivedFrame();
                    monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);
                    driver_bit_frm_2->TurnReceivedFrame();
                    driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
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