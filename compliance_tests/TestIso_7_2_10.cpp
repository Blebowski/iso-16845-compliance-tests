/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 11.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.10
 * 
 * @brief This test verifies that the IUT detects a form error when one of 6
 *        first recessive bits of EOF is forced to dominant state by LT.
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
 *      #1 to #5 corrupting the first until the fifth bit position.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 * 
 * Response:
 *  The IUT shall generate an active error frame at the bit position following
 *  the corrupted bit.
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

class TestIso_7_2_10 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 6;
            for (int i = 0; i < 6; i++)
            {
                elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::Can2_0));
                elem_tests[1].push_back(ElementaryTest(i + 1, FrameType::CanFd));
            }
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
                                    elem_tests[test_variant][elem_test.index - 1].frame_type);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);
                    
                    /*******************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received, insert ACK.
                     *   2. Force 1,2..5-th bit of EOF forced to dominant!
                     *   3. Insert Active Error frame from first bit of EOF!
                     ******************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                    driver_bit_frm->GetBitOf(elem_test.index - 1, BitType::Eof)
                        ->bit_value_ = BitValue::Dominant;

                    monitor_bit_frm->InsertActiveErrorFrame(
                        monitor_bit_frm->GetBitOf(elem_test.index, BitType::Eof));
                    driver_bit_frm->InsertActiveErrorFrame(
                        driver_bit_frm->GetBitOf(elem_test.index, BitType::Eof));

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /*******************************************************************************
                     * Execute test
                     *******************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    /* Check no frame is received by DUT */
                    if (dut_ifc->HasRxFrame())
                        test_result = false;
                }
            }

            return (int)FinishTest();
        }
};