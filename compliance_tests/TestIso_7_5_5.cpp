/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 30.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.5.5
 * 
 * @brief The purpose of this test is to verify that an error passive IUT
 *        restarts the passive error flag when detecting up to 5 consecutive
 *        dominant bits during its own passive error flag.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Passive error flag, FDF = 0
 * 
 *  CAN FD Enabled
 *      Passive error flag, FDF = 1
 * 
 * Elementary test cases:
 *  Elementary tests to perform:
 *      Elementary tests to perform superimposing the passive error flag by
 *      the sequence of 5 dominant bits starting at
 *          #1 the first bit of the passive error flag,
 *          #2 the third bit of the passive error flag, and
 *          #3 the sixth bit of the passive error flag.
 *
 * Setup:
 *  The IUT is set in passive state.
 * 
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  During the passive error flag sent by the IUT, the LT sends a sequence
 *  of 5 dominant bits according to elementary test cases.
 *  After this sequence, the LT waits for (6 + 7) bit time before sending a
 *  dominant bit, corrupting the last bit of the error delimiter.
 * 
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the last dominant bit sent by the LT.
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

class TestIso_7_5_5 : public test_lib::TestBase
{
    public:
        
        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 3;
            for (int i = 0; i < 3; i++)
            {
                elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::Can2_0));
                elem_tests[1].push_back(ElementaryTest(i + 1, FrameType::CanFd));
            }

            dut_ifc->SetTec((rand() % 110) + 128);
        }

        int Run()
        {
            SetupTestEnvironment();
            uint8_t data_byte = 0x80;

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                                    IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                                    EsiFlag::ErrorPassive);
                    golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff
                     *      bit therefore causing error.
                     *   3. Insert Passive Error frame to both driven and monitored frames from
                     *      next bit on!
                     *   4. Insert 5 dominant bits to driven frame from 1/3/6-th bit of passive
                     *      error flag.
                     *   5. Insert passive error flag from one bit beyond the last dominat bit
                     *      from previous step. Insert to both driven and monitored frames.
                     *   6. Flip last bit of new error delimiter to dominant (overload flag).
                     *   7. Insert overload flag expected from next bit on to both driven and
                     *      monitored frames.
                     *****************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
                    monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

                    int where_to_insert;
                    if (elem_test.index == 1)
                        where_to_insert = 0;
                    else if (elem_test.index == 2)
                        where_to_insert = 2;
                    else
                        where_to_insert = 5;
                    int bit_index = driver_bit_frm->GetBitIndex(
                        driver_bit_frm->GetBitOf(where_to_insert, BitType::PassiveErrorFlag));

                    for (int i = 0; i < 5; i++)
                    {
                        driver_bit_frm->InsertBit(Bit(BitType::ActiveErrorFlag, BitValue::Dominant,
                            frame_flags.get(), &nominal_bit_timing, &data_bit_timing), bit_index);
                        monitor_bit_frm->InsertBit(Bit(BitType::PassiveErrorFlag, BitValue::Recessive,
                            frame_flags.get(), &nominal_bit_timing, &data_bit_timing), bit_index);
                    }

                    /* Next Passive Error flag should start right after 5 inserted bits */
                    driver_bit_frm->InsertPassiveErrorFrame(bit_index + 5);
                    monitor_bit_frm->InsertPassiveErrorFrame(bit_index + 5);

                    /* 
                     * Now the only bits of error delimiter should be the ones from last error
                     * delimiter because it overwrote previous ones!
                     */
                    driver_bit_frm->GetBitOf(7, BitType::ErrorDelimiter)->FlipBitValue();

                    driver_bit_frm->InsertOverloadFrame(0, BitType::Intermission);
                    monitor_bit_frm->InsertOverloadFrame(0, BitType::Intermission);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();
                }
            }

            return (int)FinishTest();
        }
};