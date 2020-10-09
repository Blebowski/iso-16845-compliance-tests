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
 * @test ISO16845 7.4.6
 * 
 * @brief This test verifies that the IUT generates an overload frame when
 *        detecting a dominant bit on one of the 2 first recessive bits of
 *        the intermission field.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Intermission field, FDF = 0
 * 
 *  CAN FD Enabled
 *      Intermission field, FDF = 1
 * 
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 intermission field bit 1 dominant;
 *          #2 intermission field bit 2 dominant.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  One test frame is used for each of the two elementary tests. The LT causes
 *  the IUT to generate an error frame in data field.
 *  The LT forces one of the 2 first bits of the intermission field after the
 *  previous error delimiter of the test frame to a dominant value according to
 *  elementary test cases.
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

class TestIso_7_4_6 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 2;
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
                     *   2. Flip 7-th bit of data byte to dominant. This should be a recessive
                     *      stuff bit. Insert active error frame from next bit on to monitored
                     *      frame. Insert passive frame to driven frame (TX/RX feedback enabled).
                     *   3. Flip 1/2 bit of Intermission after error frame to dominant. Insert expected
                     *      overload frame from next bit on.
                     *********************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
                    driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

                    driver_bit_frm->GetBitOf(elem_test.index - 1, BitType::Intermission)->FlipBitValue();

                    monitor_bit_frm->InsertOverloadFrame(elem_test.index, BitType::Intermission);
                    driver_bit_frm->InsertPassiveErrorFrame(elem_test.index, BitType::Intermission);

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