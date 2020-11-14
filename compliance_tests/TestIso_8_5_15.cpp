/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.15
 *
 * @brief The purpose of this test is to verify that an active IUT changes to
 *        an error passive IUT detecting an error is at most 17 bit times.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          FDF = 0
 *      CAN FD enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform:
 *          #1 The LT check that the repeated frame start 6 + 8 + 3 + 8 bit
 *             after the last dominant bit send by LT.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT corrupts a bit in data field of this frame and then, the LT
 *  corrupts following error flag to recessive for 16 bit times causing the IUT
 *  to generate a passive error flag.
 *  The LT receives the repeated frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate a passive error flag and repeat the frame
 *  6 + 8 + 3 + 8 bit after the last dominant bit sent by LT.
 * 
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_8_5_15 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));            
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));

            /* Basic settings where IUT is transmitter */
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            /* 
             * TX/RX feedback cant be enabled since we corrupt dominant
             * transmitted bits to recessive.
             */
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

                    frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type,
                                    IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                                    EsiFlag::ErrorPassive);
                    golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, 0x1, &data_byte);

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /* Second frame differs in ESI bit */
                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);                    
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Force 7-th data bit to dominant to cause stuff error.
                     *   2. Remove all bits from next bit on.
                     *   3. Insert 16 recessive bits to driven frame. Inssert 16 dominant bits
                     *      to monitored frame.
                     *   4. Append Passive Error frame after the bits from previous step. Append
                     *      to both driven and monitored frames.
                     *   5. Append Suspend transmission to both driven and monitored frames.
                     *   6. Append next frame as if retransmitted by IUT.
                     *****************************************************************************/
                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    driver_bit_frm->RemoveBitsFrom(7, BitType::Data);
                    monitor_bit_frm->RemoveBitsFrom(7, BitType::Data);

                    /* Append 17, but last will be over-written by next passive error frame! */
                    for (int i = 0; i < 17; i++)
                    {
                        driver_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Recessive);
                        monitor_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Dominant);
                    }

                    driver_bit_frm->InsertPassiveErrorFrame(16, BitType::ActiveErrorFlag);
                    monitor_bit_frm->InsertPassiveErrorFrame(16, BitType::ActiveErrorFlag);

                    driver_bit_frm->AppendSuspendTransmission();
                    monitor_bit_frm->AppendSuspendTransmission();

                    driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                    driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                    monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    dut_ifc->SetTec(0);
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    StartDriverAndMonitor();
                    this->dut_ifc->SendFrame(golden_frm.get());
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();
                } 
            }

            return (int)FinishTest();
        }
};