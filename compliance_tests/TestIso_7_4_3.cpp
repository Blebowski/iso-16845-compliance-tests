/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 2.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.4.3
 * 
 * @brief This test verifies that the IUT generates an overload frame when
 *        detecting a dominant bit on the eighth bit of an error and overload
 *        delimiter it is transmitting.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Error Delimiter, Overload delimiter, FDF = 0
 * 
 *  CAN FD Enabled
 *      Error Delimiter, Overload delimiter, FDF = 1
 * 
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 apply error at the eighth bit of the error delimiter;
 *          #2 apply error at the eighth bit of the overload delimiter.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an error frame in data field or an
 *  overload frame after a data frame.
 *  The LT forces 1 bit to dominant state according to elementary test cases.
 * 
 * Response:
 *  The IUT generates an overload frame starting at the bit position following
 *  the dominant bit forced by LT.
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

class TestIso_7_4_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 2; i++)
            {
                elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::Can2_0));
                elem_tests[1].push_back(ElementaryTest(i + 1, FrameType::CanFd));
            }
            CanAgentConfigureTxToRxFeedback(true);
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

                    /**********************************************************************************
                     * Modify test frames:
                     *   1. Turn monitored frame as received.
                     *   2. Based on elementary test:
                     *      2.1 Flip 7-th bit of data byte to dominant. This should be a recessive
                     *          stuff bit. Insert active error frame from next bit on to monitored
                     *          frame. Insert passive frame to driven frame (TX/RX feedback enabled).
                     *      2.2 Flip first bit of intermission to dominant (overload flag).
                     *          Insert expected overload frame from next bit on.
                     *   3. Flip last bit of overload or error delimiter (based on previous step) to
                     *      dominant.
                     *   4. Insert expected overload frame from next bit on.
                     *********************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    if (elem_test.index == 1)
                    {
                        driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                        monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
                        driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
                    }
                    else
                    {
                        driver_bit_frm->GetBitOf(0, BitType::Intermission)->FlipBitValue();

                        monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
                        driver_bit_frm->InsertPassiveErrorFrame(1, BitType::Intermission);
                    }

                    /* Note that driver contains only passive error flags. Overload is in monitor */
                    Bit *last_delim_bit;
                    last_delim_bit = driver_bit_frm->GetBitOf(7, BitType::ErrorDelimiter);
                    last_delim_bit->FlipBitValue();

                    int bit_index = driver_bit_frm->GetBitIndex(last_delim_bit);
                    driver_bit_frm->InsertPassiveErrorFrame(bit_index + 1);
                    monitor_bit_frm->InsertOverloadFrame(bit_index + 1);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /**********************************************************************************
                     * Execute test
                     *********************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    if (elem_test.index == 1)
                        CheckNoRxFrame();
                    else
                        CheckRxFrame(*golden_frm);
                }
            }
            return (int)FinishTest();
        }
};