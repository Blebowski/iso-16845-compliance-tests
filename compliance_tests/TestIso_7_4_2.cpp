/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.4.2
 * 
 * @brief This test verifies that the IUT generates an overload frame when
 *        detecting a dominant state on the last bit of EOF.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      EOF, FDF = 0
 * 
 *  CAN FD Enabled
 *      EOF, FDF = 1
 * 
 * Elementary test cases:
 *      #1 Last bit of the EOF.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT forces 1 bit of the EOF to a dominant state according to elementary
 *  test cases.
 * 
 * Response:
 *  The IUT generates an overload frame at the bit position following the
 *  dominant bit.
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

class TestIso_7_4_2 : public test_lib::TestBase
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

                frame_flags = std::make_unique<FrameFlags>(
                        elem_tests[test_variant][0].frame_type);
                golden_frm = std::make_unique<Frame>(*frame_flags);
                RandomizeAndPrint(golden_frm.get());

                driver_bit_frm = ConvertBitFrame(*golden_frm);
                monitor_bit_frm = ConvertBitFrame(*golden_frm);

                TestMessage("Forcing last bit of EOF to dominant!");

                /**********************************************************************************
                 * Modify test frames:
                 *   1. Monitor frame as if received, insert ACK to driven frame.
                 *   2. Force last bit of EOF to DOMINANT.
                 *   3. Insert expected overload frame from first bit of Intermission.
                 *********************************************************************************/
                monitor_bit_frm->TurnReceivedFrame();
                driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                driver_bit_frm->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

                monitor_bit_frm->InsertOverloadFrame(0, BitType::Intermission);
                driver_bit_frm->InsertOverloadFrame(0, BitType::Intermission);

                driver_bit_frm->Print(true);
                monitor_bit_frm->Print(true);

                /**********************************************************************************
                 * Execute test
                 *********************************************************************************/
                PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                RunLowerTester(true, true);
                CheckLowerTesterResult();

                CheckRxFrame(*golden_frm);
            }
            return (int)FinishTest();
        }
};