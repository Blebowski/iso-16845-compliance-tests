/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.3
 * 
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting 8 consecutive dominant bits following the
 *        transmission of its active error flag and after each sequence of
 *        additional 8 consecutive dominant bits.
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
 *      #1 After the error flag sent by the IUT, the LT sends a sequence of 16
 *         dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an active error frame in data field
 *  according to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be increased by 8 on each eighth dominant bit
 *  after the error flag.
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

class TestIso_8_6_3 : public test_lib::TestBase
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
            uint8_t data_byte = 0x80;

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                                    IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                                    EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /* Second frame the same due to retransmission. */
                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);                    
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Force 7-th data bit to dominant to cause stuff error.
                     *   2. Insert Active Error frame from next bit on to monitored frame.
                     *      Insert Passive Error frame to driven frame.
                     *   3. Insert 16 dominant bits to driven frame after the end of error flag.
                     *      Insert 16 recessive bits to monitored frame.
                     *   4. Append retransmitted frame!
                     *****************************************************************************/
                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
                    monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

                    for (int i = 0; i < 16; i++)
                    {
                        int bit_index = driver_bit_frm->GetBitIndex(
                                            driver_bit_frm->GetBitOf(5, BitType::PassiveErrorFlag));
                        driver_bit_frm->InsertBit(Bit(BitType::ActiveErrorFlag, BitValue::Dominant,
                                                        frame_flags.get(), &nominal_bit_timing,
                                                        &data_bit_timing),
                                                    bit_index + 1);
                        monitor_bit_frm->InsertBit(Bit(BitType::PassiveErrorFlag, BitValue::Recessive,
                            frame_flags.get(), &nominal_bit_timing, &data_bit_timing), bit_index + 1);
                    }

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

                    /* 8 for first Error frame, 2 * 8 for 16 bits, - 1 for succesfull retransmission */
                    CheckTecChange(tec_old, 23);
                }
            }

            return (int)FinishTest();
        }
};