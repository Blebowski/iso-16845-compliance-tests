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
 * @test ISO16845 7.2.11
 * 
 * @brief The purpose of this test is to verify the point of time at which a
 *        message is still considered as non-valid by the IUT.
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
 *      #1 The sixth bit of the EOF is forced to dominant.
 *
 * Setup:
 *  The IUT has to be initialized with data different from those used in the
 *  test frame.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with form error at EOF according to elementary
 *  test cases.
 * 
 * Response:
 *  The IUT shall generate an active error frame.
 *  The data initialized during the set-up state shall remain unchanged.
 *  DontShift frame reception shall be indicated to the upper layers of the IUT.
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

class TestIso_7_2_11 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));
        }

        int Run()
        {
            SetupTestEnvironment();

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                for (auto elem_test : elem_tests[test_variant])
                {
                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);
                
                    /**********************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received, insert ACK.
                     *   2. 6-th bit of EOF forced to dominant!
                     *   3. Insert Active Error frame from first bit of EOF!
                     *********************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                    driver_bit_frm->GetBitOf(5, BitType::Eof)->bit_value_ = BitValue::Dominant;

                    monitor_bit_frm->InsertActiveErrorFrame(6, BitType::Eof);
                    driver_bit_frm->InsertActiveErrorFrame(6, BitType::Eof);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /********************************************************************************** 
                     * Execute test
                     *********************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();
                    CheckNoRxFrame();
                }
            }

            return (int)FinishTest();
        }
};