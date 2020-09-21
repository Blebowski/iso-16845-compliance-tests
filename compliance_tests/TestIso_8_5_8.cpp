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
            elem_tests[0].push_back(ElementaryTest(1 , FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));

            dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        int Run()
        {
            SetupTestEnvironment();

            /* To avoid stuff bits causing mismatches betwen frame lengths */
            uint8_t data_byte = 0xAA;

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                /* ESI needed for CAN FD variant */
                frame_flags = std::make_unique<FrameFlags>(elem_tests[test_variant][0].frame_type,
                                IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                                EsiFlag::ErrorPassive);
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

                /******************************************************************************
                 * Modify test frames:
                 *   1. Loose arbitration on monitored frame on first bit (0x245 vs 0x445 IDs).
                 *   2. Do iteration specific compensation due to different number of stuff
                 *      bits (since there are different IDs and CRCs):
                 *        A. Common variant - remove one bit
                 *   3. Append the same frame, retransmitted.
                 *****************************************************************************/
                monitor_bit_frm->LooseArbitration(
                    monitor_bit_frm->GetBitOf(0, BitType::BaseIdentifier));

                if (test_variants[test_variant] == TestVariant::Common)
                    monitor_bit_frm->RemoveBit(monitor_bit_frm->GetBitOf(0, BitType::Data));

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
            }

            return (int)FinishTest();
        }
};