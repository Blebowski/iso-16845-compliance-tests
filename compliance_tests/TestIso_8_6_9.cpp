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
 * @test ISO16845 8.6.9
 * 
 * @brief This test verifies that an IUT acting as a transmitter increases its
 *        TEC by 8 when detecting a form error on a bit of the error delimiter
 *        it is transmitting.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      FDF = 0
 * 
 *  CAN FD Enabled
 *      ACk Slot 2 bits
 *      FDF = 1
 * 
 * Elementary test cases:
 *   Elementary tests to perform:
 *     #1 corrupting the second bit of the error delimiter;
 *     #2 corrupting the fourth bit of the error delimiter;
 *     #3 corrupting the seventh bit of the error delimiter.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an error frame in data field.
 *  The LT corrupts the error delimiter according to elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be increased by 8 at the corrupted bit.
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

class TestIso_8_6_9 : public test_lib::TestBase
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

                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Flip 7-th data bit to dominant, this should be Recessive stuff bit.
                     *   2. Insert Active Error frame from next bit on to monitored frame. Insert
                     *      passive Error frame to driven frame.
                     *   3. Flip 2,4 or 7-th bit of error delimiter to dominant.
                     *   4. Insert next error frame from one bit later!
                     *   5. Append retransmitted frame.
                     *****************************************************************************/
                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
                    monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

                    int bit_to_flip;
                    if (elem_test.index == 1)
                        bit_to_flip = 1;
                    else if (elem_test.index == 2)
                        bit_to_flip = 3;
                    else
                        bit_to_flip = 6;
                    int bit_index = driver_bit_frm->GetBitIndex(
                        driver_bit_frm->GetBitOf(bit_to_flip, BitType::ErrorDelimiter));
                    driver_bit_frm->GetBit(bit_index)->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->InsertPassiveErrorFrame(bit_index + 1);
                    monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

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

                    /* +8 for original error frame, +8 for new one, -1 for retransmission! */
                    CheckTecChange(tec_old, 15);
                }
            }

            return (int)FinishTest();
        }
};