/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.6.17
 * 
 * @brief This test verifies that a passive state IUT acting as a transmitter
 *        does not increase its TEC when detecting an acknowledgement error
 *        followed by a passive error flag.
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
 *     #1 ACK = recessive
 * 
 * Setup:
 *  The IUT is set to the TEC passive.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT sends acknowledgement for this frame according to elementary
 *  test cases.
 *  After the acknowledgement error, the LT sends a passive error frame.
 *  
 * Response:
 *  The IUT’s TEC value shall be equal to the set-up value.
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

class TestIso_8_6_17 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            dut_ifc->SetTec((rand() % 125) + 130);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force ACK to recessive value.
             *   3. Insert Passive Error Frame to both driven and monitored frames from ACK deli-
             *      miter further.
             *   4. Append suspend transmission since IUT is Error passive!
             *   5. Insert retransmitted frame, but with ACK set.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

            driver_bit_frm->InsertPassiveErrorFrame(0, BitType::AckDelimiter);
            monitor_bit_frm->InsertPassiveErrorFrame(0, BitType::AckDelimiter);

            driver_bit_frm->AppendSuspendTransmission();
            monitor_bit_frm->AppendSuspendTransmission();

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();
            /* +0 for ACK Error, -1 for succesfull retransmission! */
            CheckTecChange(tec_old, -1);

            return FinishElementaryTest();
        }

};