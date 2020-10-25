/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.21
 * 
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when transmitting a frame successfully.
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
 *   There is one elementary test to perform:
 *      #1 The higher prior frame is disturbed by an error to increase REC.
 *
 * Setup:
 *  No action required, the IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *
 *  The LT sends a frame with higher ID priority to cause the IUT to lose
 *  arbitration according to elementary test cases. The IUT will repeat its
 *  transmission after error treatment.
 *  
 * Response:
 *  The IUT’s REC value shall be incremented and not decremented after
 *  transmission.
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

class TestIso_7_6_21 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);

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
                                    IdentifierType::Base, RtrFlag::DataFrame,
                                    /* Dont shift needed since Transmitted frame after received
                                     * frame is not handled well with Bit rate shifts due to
                                     * small resynchronizations in reciver!
                                     */
                                    BrsFlag::DontShift, EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0xAB, &data_byte);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /* Second frame the same due to retransmission. */
                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);                    
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Flip driven frame to dominant one one bit before end of Base ID. This
                     *      bit should be last recessive bit of Base ID.
                     *   2. Loose arbitration on monitored frame from the same bit
                     *   3. Flip 7-th bit of data field to dominant. This causes stuff error.
                     *   4. Insert Active Error frame from next bit on to driven frame. Insert
                     *      Passive Error frame to monitored frame.
                     *****************************************************************************/
                    Bit *loosing_bit = driver_bit_frm->GetBitOf(9, BitType::BaseIdentifier);
                    loosing_bit->bit_value_ = BitValue::Dominant;
                    int bit_index = driver_bit_frm->GetBitIndex(loosing_bit);

                    monitor_bit_frm->LooseArbitration(bit_index);

                    driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

                    driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
                    monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

                    driver_bit_frm_2->TurnReceivedFrame();
                    driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                    monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    dut_ifc->SetRec(20);
                    tec_old = dut_ifc->GetTec();
                    rec_old = dut_ifc->GetRec();
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    StartDriverAndMonitor();
                    dut_ifc->SendFrame(golden_frm.get());
                    WaitForDriverAndMonitor();

                    CheckLowerTesterResult();
                    CheckRecChange(rec_old, +1);
                    CheckTecChange(tec_old, +0);
                }
            }

            return (int)FinishTest();
        }
};