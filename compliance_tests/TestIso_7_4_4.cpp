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
 * @test ISO16845 7.4.4
 * 
 * @brief This test verifies that the IUT detects a bit error when one of the
 *        6 dominant bits of the overload flag it transmits is forced to
 *        recessive state by LT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Overload flag, FDF = 0
 * 
 *  CAN FD Enabled
 *      Overload flag, FDF = 1
 * 
 * Elementary test cases:
 *      #1 corrupting the first bit of the overload flag;
 *      #2 corrupting the third bit of the overload flag;
 *      #3 corrupting the sixth bit of the overload flag.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  The LT forces 1 bit of the overload flag to the recessive state according
 *  to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame at the bit position following the
 *  corrupted bit.
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

class TestIso_7_4_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 3; i++)
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

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Force last bit of EOF to Dominant!
                     *   3. Insert Overload frame from first bit of Intermission.
                     *   4. Flip 1/3/6 bit of Overload flag to RECESSIVE!
                     *   5. Insert Active Error frame to both monitored and driven
                     *      frame!
                     *****************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();
                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

                    monitor_bit_frm->InsertOverloadFrame(0, BitType::Intermission);
                    driver_bit_frm->InsertOverloadFrame(0, BitType::Intermission);

                    /* Force n-th bit of Overload flag on can_rx (driver) to RECESSIVE */
                    int bit_to_corrupt;
                    if (elem_test.index == 1)
                        bit_to_corrupt = 1;
                    else if (elem_test.index == 2)
                        bit_to_corrupt = 3;
                    else
                        bit_to_corrupt = 6;

                    TestMessage("Forcing Overload flag bit %d to recessive", bit_to_corrupt);

                    Bit *bit = driver_bit_frm->GetBitOf(bit_to_corrupt - 1, BitType::OverloadFlag);
                    int bit_index = driver_bit_frm->GetBitIndex(bit);
                    bit->bit_value_ = BitValue::Recessive;

                    /* Insert Error flag from one bit further, both driver and monitor! */
                    driver_bit_frm->InsertActiveErrorFrame(bit_index + 1);
                    monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /******************************************************************************
                     * Execute test
                     *****************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();

                    /*
                     * Receiver will make received frame valid on 6th bit of EOF! Therefore at
                     * point where Error occurs, frame was already received OK and should be
                     * readable!
                     */
                    CheckRxFrame(*golden_frm);
                }
            }
            return (int)FinishTest();
        }
};