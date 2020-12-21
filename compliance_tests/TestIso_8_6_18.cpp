/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 28.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.18
 * 
 * @brief This test verifies that a passive state IUT acting as a transmitter
 *        increases its TEC when detecting an acknowledgement error followed
 *        by at least 1 dominant bit during the passive error flag.
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
 *     #1 LT send dominant bit at the sixth bit position of the passive error
 *        flag.
 * 
 * Setup:
 *  The IUT is set to the TEC passive.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT does not acknowledge this frame.
 *  After the acknowledgement error, the LT sends a dominant bit according to
 *  elementary test cases.
 *  
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value increased by 8 for
 *  the error in passive error flag.
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

class TestIso_8_6_18 : public test_lib::TestBase
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

            /* Up to +16 during test. Must restrict to stay passive, not bu off! */
            dut_ifc->SetTec((rand() % 125) + 110);
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
                                    EsiFlag::ErrorPassive);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Turn driven frame as if received.
                     *   2. Force ACK to recessive value.
                     *   3. Insert next ACK for CAN FD variant since in CAN FD nodes, IUT shall
                     *      tolerate up to 1 recessive ACK and detect error only upon second
                     *      recessive bit
                     *   4. Insert Passive Error Frame to both driven and monitored frames from
                     *      ACK delimiter further.
                     *   5. Force last bit of Error flag to dominant. Note that this does not cause
                     *      next error frame since it is detection of Dominant during Passive
                     *      Error flag! It shall, however lead to increment of TEC! This directly
                     *      tests exception 1 of rule "c" of "Error counting" in ISO11898-1 2015!
                     *   6. Append suspend transmission since IUT is Error passive!
                     *   7. Insert retransmitted frame, but with ACK set.
                     *****************************************************************************/
                    driver_bit_frm->TurnReceivedFrame();
                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

                    if (test_variants[test_variant] == TestVariant::CanFdEnabled)
                    {
                        int index = driver_bit_frm->GetBitIndex(
                            driver_bit_frm->GetBitOf(0, BitType::Ack));
                        driver_bit_frm->InsertBit(BitType::Ack, BitValue::Recessive, index);
                        monitor_bit_frm->InsertBit(BitType::Ack, BitValue::Recessive, index);
                    }

                    driver_bit_frm->InsertPassiveErrorFrame(0, BitType::AckDelimiter);
                    monitor_bit_frm->InsertPassiveErrorFrame(0, BitType::AckDelimiter);

                    Bit *last_err_flg_bit = driver_bit_frm->GetBitOf(5, BitType::PassiveErrorFlag);
                    last_err_flg_bit->FlipBitValue();

                    driver_bit_frm->AppendSuspendTransmission();
                    monitor_bit_frm->AppendSuspendTransmission();

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

                    /* 
                     * +8 for ACK error followed by dominant bit during passive error flag,
                     * -1 for succesfull retransmission!
                     */
                    CheckTecChange(tec_old, +7);
                }
            }

            return (int)FinishTest();
        }
};