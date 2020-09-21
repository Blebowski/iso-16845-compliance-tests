/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 6.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.1
 * 
 * @brief This test verifies that the IUT detects a bit error when the dominant
 *        ACK slot is forced to recessive state by LT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ACK Slot, FDF = 0
 * 
 *  CAN FD Enabled
 *      ACK Slot, FDF = 1
 * 
 * Elementary test cases:
 *      #1 The dominant acknowledgement bit sent by the IUT is forced to
 *         recessive state.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 * 
 * Response:
 *  The IUT shall generate an active error frame starting at the bit position
 *  following the bit error.
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

class TestIso_7_2_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 1;
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));
        }

        int Run()
        {
            SetupTestEnvironment();

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                frame_flags = std::make_unique<FrameFlags>(elem_tests[test_variant][0].frame_type);
                golden_frm = std::make_unique<Frame>(*frame_flags);
                RandomizeAndPrint(golden_frm.get());

                driver_bit_frm = ConvertBitFrame(*golden_frm);
                monitor_bit_frm = ConvertBitFrame(*golden_frm);

                /**********************************************************************************
                 * Modify test frames:
                 *   1. Monitor frame as if received.
                 *   2. Insert error frame to monitored/driven frame at position
                 *      of ACK delimiter since driver does not have ACK dominant!
                 *********************************************************************************/
                monitor_bit_frm->TurnReceivedFrame();

                monitor_bit_frm->InsertActiveErrorFrame(
                    monitor_bit_frm->GetBitOf(0, BitType::AckDelimiter));
                driver_bit_frm->InsertActiveErrorFrame(
                    driver_bit_frm->GetBitOf(0, BitType::AckDelimiter));

                /********************************************************************************** 
                 * Execute test
                 *********************************************************************************/
                PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                RunLowerTester(true, true);
                CheckLowerTesterResult();
            }

            return (int)FinishTest();
        }
};