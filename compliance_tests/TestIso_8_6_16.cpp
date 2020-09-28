/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 25.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.16
 * 
 * @brief This test verifies that an IUT acting as a transmitter does not
 *        change the value of its TEC when detecting a form error on the last
 *        bit of the error delimiter it is transmitting.
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
 *     #1 LT sends 1 dominant bit.
 * 
 * Setup:
 *  The LT forces the IUT to increase its TEC.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame and causes the IUT to send an
 *  active error flag in data filed.
 *  After the last bit of the error flag, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be 8.
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

class TestIso_8_6_16 : public test_lib::TestBase
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

            dut_ifc->SetTec(8);
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

                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Turn driven frame as if received.
                     *   2. Corrupt 7-th bit of data field. This should be recessive stuff bit.
                     *      Force it to dominant.
                     *   3. Insert Active Error frame to monitored frame from next bit on.
                     *      Insert Passive Error frame to driven frame from next bit on.
                     *   4. Flip last bit of Error delimiter (8-th) to dominant in driven frame.
                     *   5. Insert Overload frame from next bit on. Insert Passive error frame
                     *      from next bit on.
                     *   6. Append retransmitted frame.
                     *****************************************************************************/
                    driver_bit_frm->TurnReceivedFrame();

                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
                    monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

                    driver_bit_frm->GetBitOf(7, BitType::ErrorDelimiter)->FlipBitValue();

                    driver_bit_frm->InsertPassiveErrorFrame(0, BitType::Intermission);
                    monitor_bit_frm->InsertOverloadFrame(0, BitType::Intermission);

                    driver_bit_frm_2->TurnReceivedFrame();
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

                    /* +1 for original error frame, -1 for succesfull retransmission */
                    CheckTecChange(tec_old, +7);
                }
            }

            return (int)FinishTest();
        }
};