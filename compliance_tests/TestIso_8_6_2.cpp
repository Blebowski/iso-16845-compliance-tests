/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 19.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.2
 * 
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting a bit error during the transmission of an
 *        overload flag.
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
 *   Elementary tests to perform:
 *      #1 corrupting the first bit of the overload flag;
 *      #2 corrupting the fourth bit of the overload flag;
 *      #3 corrupting the sixth bit of the overload flag.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame after a data
 *  frame.
 *  The LT corrupts one of the dominant bits of the overload flag according
 *  to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be increased by 8 on the corrupted bit.
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

class TestIso_8_6_2 : public test_lib::TestBase
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

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
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

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                                                                EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Insert ACK to driven frame (TX/RX feedback disabled)
                     *   2. Force first bit of intermission low (overload condition)
                     *   3. Corrupt 1,3,6-th bit of overload flag.
                     *   4. Insert Active error frame from one bit further.
                     *****************************************************************************/
                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->GetBitOf(0, BitType::Intermission)
                        ->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
                    monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

                    int bit_index_to_corrupt;
                    if (elem_test.index == 1)
                        bit_index_to_corrupt = 0;
                    else if (elem_test.index == 2)
                        bit_index_to_corrupt = 2;
                    else
                        bit_index_to_corrupt = 5;

                    Bit *bit_to_corrupt = driver_bit_frm->GetBitOf(bit_index_to_corrupt,
                                                                    BitType::OverloadFlag);
                    bit_to_corrupt->bit_value_ = BitValue::Recessive;
                    
                    int bit_index = driver_bit_frm->GetBitIndex(bit_to_corrupt);
                    driver_bit_frm->InsertActiveErrorFrame(bit_index + 1);
                    monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    tec_old = dut_ifc->GetTec();
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    StartDriverAndMonitor();
                    dut_ifc->SendFrame(golden_frm.get());
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();

                    /* 8 for Error frame, -1 for sucesfull transmision! */
                    if (test_variant == 0 && elem_test.index == 1)
                        CheckTecChange(tec_old, 8);
                    else
                        CheckTecChange(tec_old, 7);
                }
            }

            return (int)FinishTest();
        }
};