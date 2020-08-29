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
 * @test ISO16845 7.3.1
 * 
 * @brief This test verifies that the IUT tolerates up to 7 consecutive
 *        dominant bits after sending an active error flag.
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
 *      #1 lengthening the error flag by 1 dominant bit;
 *      #2 lengthening the error flag by 4 dominant bits;
 *      #3 lengthening the error flag by 7 dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field.
 *  The LT lengthens the error flag generated by the IUT according to
 *  elementary test cases.
 * 
 * Response:
 *  After sending the active error flag, the IUT sends recessive bits.
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

class TestIso_7_3_1 : public test_lib::TestBase
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
                        elem_tests[test_variant][elem_test.index].frame_type,
                        RtrFlag::DataFrame);
                    golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    TestMessage("Prolonging Active Error flag by: %d", (3 * (elem_test.index - 1)) + 1);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit!
                     *      This will cause stuff error!
                     *   3. Insert Active Error frame from 8-th bit of data frame!
                     *   4. Prolong Active error flag by 1,4,7 bits respectively.
                     *      Prolong Monitored error delimier by 1,4,7 Recessive bits!
                     *****************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    monitor_bit_frm->InsertActiveErrorFrame(
                        monitor_bit_frm->GetBitOf(7, BitType::Data));
                    driver_bit_frm->InsertActiveErrorFrame(
                        driver_bit_frm->GetBitOf(7, BitType::Data));

                    int num_bits_to_insert;
                    if (elem_test.index == 1)
                        num_bits_to_insert = 1;
                    else if (elem_test.index == 2)
                        num_bits_to_insert = 4;
                    else
                        num_bits_to_insert = 7;

                    /* Prolong driven frame by 1,4,7 DOMINANT bits */
                    int drv_last_err_flg_index = driver_bit_frm->GetBitIndex(
                        driver_bit_frm->GetBitOf(5, BitType::ActiveErrorFlag));
                    for (int k = 0; k < num_bits_to_insert; k++)
                        driver_bit_frm->InsertBit(
                            Bit(BitType::ActiveErrorFlag, BitValue::Dominant, frame_flags.get(),
                                &nominal_bit_timing, &data_bit_timing),
                            drv_last_err_flg_index);

                    /* Prolong monitored frame by 1,4,7 RECESSIVE bits */
                    int mon_last_err_flg_index = monitor_bit_frm->GetBitIndex(
                        monitor_bit_frm->GetBitOf(0, BitType::ErrorDelimiter));
                    for (int k = 0; k < num_bits_to_insert; k++)
                        monitor_bit_frm->InsertBit(
                            Bit(BitType::ErrorDelimiter, BitValue::Recessive, frame_flags.get(),
                                &nominal_bit_timing, &data_bit_timing),
                            mon_last_err_flg_index);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /********************************************************************************** 
                     * Execute test
                     *********************************************************************************/
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