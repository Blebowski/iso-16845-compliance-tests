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
 * @test ISO16845 7.2.6
 * 
 * @brief The purpose of this test is to verify that an IUT detecting a CRC
 *        error and a form error on the CRC delimiter in the same frame
 *        generates only one single 6 bits long error flag starting on the bit
 *        following the CRC delimiter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter, FDF = 0
 * 
 *  CAN FD Enabled
 *      CRC, DLC - to cause different CRC types. FDF = 1
 * 
 * Elementary test cases:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      #1 CRC (15)
 *
 *  CAN FD Enabled
 *      #1 DLC ≤ 10 − > CRC (17)
 *      #2 DLC > 10 − > CRC (21)
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with CRC error and form error at CRC delimiter
 *  according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate one active error frame starting at the bit position
 *  following the CRC delimiter.
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

class TestIso_7_2_6: public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 2;
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));
            elem_tests[1].push_back(ElementaryTest(2, FrameType::CanFd));
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

                    uint8_t dlc;
                    if (test_variant == 0)
                    {
                        dlc = rand() % 9;
                    } else if (elem_test.index == 1)
                    {
                        if (rand() % 2)
                            dlc = 0x9;
                        else
                            dlc = 0xA;
                    } else
                    {
                        dlc = (rand() % 5) + 11; 
                    }

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type);
                    golden_frm = std::make_unique<Frame>(*frame_flags, dlc);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);
                
                    /******************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Force random CRC bit to its opposite value
                     *   3. Force CRC Delimiter to dominant.
                     *   4. Insert Error frame to position of ACK!
                     *****************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    int crc_index;
                    if (test_variants[test_variant] == TestVariant::Common)
                        crc_index = rand() % 15;
                    else if (elem_test.index == 1)
                        crc_index = rand() % 17;
                    else
                        crc_index = rand() % 21;

                    TestMessage("Forcing CRC bit nr: %d", crc_index);
                    driver_bit_frm->GetBitOfNoStuffBits(crc_index, BitType::Crc)->FlipBitValue();

                    /* 
                     * TODO: Here we should re-stuff CRC because we might have added/removed
                     *       Stuff bit in CRC and causes length of model CRC and to be different!
                     */
                    driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter)->bit_value_ = BitValue::Dominant;

                    monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Ack);
                    driver_bit_frm->InsertActiveErrorFrame(0, BitType::Ack);

                    /******************************************************************************* 
                     * Execute test
                     ******************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();
                }
            }

            return (int)FinishTest();
        }
};