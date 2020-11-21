/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 21.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.1.2
 * 
 * @brief The purpose of this test is to verify the sample point of an IUT
 *        acting as a transmitter on bit position BRS.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  CAN FD enabled
 *      Sampling_Point(N) configuration as available by IUT.
 *      BRS = 1
 *      FDF = 1
 * 
 * Elementary test cases:
 *  There is one elementary test to perform for each programmable sampling
 *  point inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 BRS bit level changed from dominant to recessive before sampling
 *         point.
 *
 *  Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation is disabled.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces Phase_Seg2(N) of “res” bit to recessive according to
 *  elementary test cases.
 * 
 * Response:
 *  The modified “res” bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur.
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

class TestIso_8_8_1_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            elem_tests[0].push_back(ElementaryTest(1));
            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
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

                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                BrsFlag::Shift, EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Insert ACK to driven frame.
                     *   2. Force SYNC + Prop + Ph1 - 1 starting time quantas of BRS to Dominant.
                     *****************************************************************************/
                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    Bit *brs_bit = driver_bit_frm->GetBitOf(0, BitType::Brs);
                    size_t num_time_quantas = nominal_bit_timing.prop_ + nominal_bit_timing.ph1_;
                    for (size_t i = 0; i < num_time_quantas; i++)
                        brs_bit->ForceTimeQuanta(i, BitValue::Dominant);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    StartDriverAndMonitor();
                    dut_ifc->SendFrame(golden_frm.get());
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();
                }
            }
            return (int)FinishTest();
        }
};