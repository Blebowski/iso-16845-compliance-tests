/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.9
 * 
 * @brief This test verifies the behaviour of the IUT when receiving two
 *        consecutive frames not separated by a bus idle state.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Intermission field length
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      Intermission field length
 *      FDF = 1
 * 
 * Elementary test cases:
 *      #1 The second frame starts after the second intermission bit of the
 *         first frame.
 *      #2 The second frame starts after the third intermission bit of the
 *         first frame.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  Two different test frames are used for each of the two elementary tests.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frames.
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

class TestIso_7_1_9 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 2;
            for (int i = 0; i < num_elem_tests; i++)
            {
                elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::Can2_0));
                elem_tests[1].push_back(ElementaryTest(i + 1, FrameType::CanFd));
            }
        }

        int Run()
        {
            SetupTestEnvironment();

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type);
                    golden_frm_2 = std::make_unique<Frame>(*frame_flags_2);
                    RandomizeAndPrint(golden_frm_2.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);
                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

                    /**********************************************************************************
                     * Modify test frames:
                     *   1. In first Elementary test, Intermission lasts only 2 bits ->
                     *      Remove last bit of intermission!
                     *   2. Monitor frames as if received, driver frame must have ACK too!
                     **********************************************************************************/
                    if (elem_test.index == 1)
                    {
                        driver_bit_frm->RemoveBit(
                            driver_bit_frm->GetBitOf(2, BitType::Intermission));
                        monitor_bit_frm->RemoveBit(
                            monitor_bit_frm->GetBitOf(2, BitType::Intermission));
                    }

                    monitor_bit_frm->TurnReceivedFrame();
                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    monitor_bit_frm_2->TurnReceivedFrame();
                    driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    /********************************************************************************** 
                     * Execute test
                     *********************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    CheckRxFrame(*golden_frm);
                    CheckRxFrame(*golden_frm_2);
                }
            }
            return (int)FinishTest();
        }
};