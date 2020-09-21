/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 21.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.8
 * 
 * @brief This test verifies that an error-active IUT acting as a transmitter
 *        increases its TEC by 8 when detecting an acknowledgement error in
 *        a frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      ACK Slot 1 bit
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      ACk Slot 2 bits
 *      FDF = 1
 * 
 * Elementary test cases:
 *   There is one elementary test to perform:
 *     #1 ACK slot = recessive
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT sends the acknowledgement slot for this frame according to
 *  elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be increased by 8 at the acknowledgement error
 *  detection.
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

class TestIso_8_6_8 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 1;
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentConfigureTxToRxFeedback(true);
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

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                                                    EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. For CAN FD insert one more recessive ACK bit (transmitter tolerates
                     *      up to two recessive bits)
                     *   2. Insert Active Error frame from ACK delimiter on to driven frame.
                     *      Insert Passive Error frame to monitored frame (TX/RX feedback enabled)
                     *   3. Append the same frame as if retransmitted!
                     *****************************************************************************/
                    if (test_variants[test_variant] == TestVariant::CanFdEnabled)
                    {
                        int bit_index = driver_bit_frm->GetBitIndex(
                            driver_bit_frm->GetBitOf(0, BitType::Ack));
                        driver_bit_frm->InsertBit(Bit(BitType::Ack, BitValue::Recessive,
                            frame_flags.get(), &nominal_bit_timing, &data_bit_timing),
                            bit_index);
                        monitor_bit_frm->InsertBit(Bit(BitType::Ack, BitValue::Recessive,
                            frame_flags.get(), &nominal_bit_timing, &data_bit_timing),
                            bit_index);
                    }

                    driver_bit_frm->InsertPassiveErrorFrame(0, BitType::AckDelimiter);
                    monitor_bit_frm->InsertActiveErrorFrame(0, BitType::AckDelimiter);

                    driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                    monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

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

                    /* +8 for ACK Error, -1 for retransmission */
                    CheckTecChange(tec_old, 7);
                }
            }

            return (int)FinishTest();
        }
};