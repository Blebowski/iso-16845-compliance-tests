/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.22
 * 
 * @brief This test verifies that the IUT increases its REC by 1 when
 *        detecting a form error.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  CAN FD Enabled
 *      REC
 *      DLC - to cause different CRC types
 *      FDF = 1
 * 
 * Elementary test cases:
 *   Elementary tests to perform on recessive stuff bits:
 *      #1 DLC ≤ 10 − > CRC (17) field;
 *      #2 DLC > 10 − > CRC (21) field.
 *   Elementary tests to perform on dominant stuff bits:
 *      #3 DLC ≤ 10 − > CRC (17) field
 *      #4 DLC > 10 − > CRC (21) field
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT corrupts a fixed stuff bit according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 on the corrupted fixed stuff
 *  bit.
 * 
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

class TestIso_7_6_22 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (int i = 0; i < 4; i++)
                elem_tests[0].push_back(ElementaryTest(i + 1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int Run()
        {
            SetupTestEnvironment();
            int num_variants_tested = 0;

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    uint8_t dlc;
                    BitValue bit_value;

                    /* Tests 1,3 -> DLC < 10. Tests 2,4 -> DLC > 10 */
                    if (elem_test.index % 2 == 0)
                        dlc = (rand() % 5) + 0xA;
                    else
                        dlc = rand() % 10;

                    if (elem_test.index < 3)
                        bit_value = BitValue::Recessive;
                    else
                        bit_value = BitValue::Dominant;

                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, RtrFlag::DataFrame);
                    golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Turn monitored frame to received frame.
                     *   2. Pick one of the stuff bits with required value (can be only in CRC field
                     *      or stuff count!) and flip its value.
                     *   3. Insert Active Error frame to monitored frame. Insert Passive Error frame
                     *      to driven frame (TX/RX feedback enabled).
                     *****************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    int num_stuff_bits = driver_bit_frm->GetNumStuffBits(StuffBitType::FixedStuffBit,
                                                                         bit_value);
                    printf("Number of fixed stuff bits matching: %d\n", num_stuff_bits);

                    /*****************************************************************************
                     * Execute test
                     ****************************************************************************/
                    for (int stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
                    {
                        TestMessage("Testing stuff bit nr: %d", stuff_bit);
                        printf("%d\n", stuff_bit);
                        num_variants_tested++;
                    
                        /* 
                         * Copy frame to second frame so that we dont loose modification of bits.
                         * Corrupt only second one.
                         */
                        driver_bit_frm_2 = std::make_unique<BitFrame>(*driver_bit_frm);
                        monitor_bit_frm_2 = std::make_unique<BitFrame>(*monitor_bit_frm);

                        /* Get stuff bit on 'stuff_bit' position */
                        Bit *stuff_bit_to_flip = driver_bit_frm_2->GetFixedStuffBit(
                                                    stuff_bit, bit_value);

                        int bit_index = driver_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                        stuff_bit_to_flip->FlipBitValue();

                        driver_bit_frm_2->InsertPassiveErrorFrame(bit_index + 1);
                        monitor_bit_frm_2->InsertActiveErrorFrame(bit_index + 1);

                        driver_bit_frm_2->Print(true);
                        monitor_bit_frm_2->Print(true);

                        /* Test itself */
                        rec_old = dut_ifc->GetRec();
                        PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
                        RunLowerTester(true, true);

                        CheckLowerTesterResult();
                        CheckRecChange(rec_old, +1);
                        if (test_result == false)
                            return false;
                    }
                }
                TestBigMessage("Tested %d stuff bits in this variant!", num_variants_tested);
                printf("%d\n", num_variants_tested);
            }

            return (int)FinishTest();
        }
};