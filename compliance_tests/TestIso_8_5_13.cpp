/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 1.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.13
 *
 * @brief The purpose of this test is to verify that an error passive IUT
 *        acting as a transmitter detects a form error when monitoring a
 *        corruption in the error delimiter.
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
 *      Elementary tests to perform:
 *          #1 corrupting the second bit of the error delimiter;
 *          #2 corrupting the fourth bit of the error delimiter;
 *          #3 corrupting the seventh bit of the error delimiter.
 * 
 * Setup:
 *  The IUT is set to the TEC passive state
 *
 * Execution:
 *  The LT causes the IUT to transmit a data frame.
 *  Then, the LT corrupt a bit in data field to cause the IUT to generate
 *  a passive error frame.
 *  The LT creates a form error according to elementary test cases.
 *  After the form error, the LT waits for (6 + 7) bit time before sending
 *  a dominant bit.
 *
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the last dominant bit generated by the LT.
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

class TestIso_8_5_13 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));            
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            /* Basic settings where IUT is transmitter */
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);

            /* To be error passive */
            dut_ifc->SetTec(160);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base,
                            RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* Second frame the same due to retransmission. */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);                    
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force 7-th data bit to dominant to cause stuff error.
             *   2. Insert Passive Error frame to both driven and monitored frames from next bit on.
             *   3. Flip 2,4 or 7-th bit of Error delimiter in driven frame.
             *   4. Insert next Passive Error flag from next bit on to both driven and monitored
             *      frames.
             *   5. Flip first intermission bit (should be 6+7 bits after last flipped bit) to
             *      dominant.
             *   6. Insert Overload frame from from next bit on (in monitored frame).
             *   7. Append Suspend transmission.
             *   8. Append retransmitted frame by IUT.
             *************************************************************************************/
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            int index_to_flip;
            if (elem_test.index == 1)
                index_to_flip = 2;
            else if (elem_test.index == 2)
                index_to_flip = 4;
            else
                index_to_flip = 7;

            Bit *bit_to_flip = driver_bit_frm->GetBitOf(index_to_flip - 1, BitType::ErrorDelimiter);
            int global_index = driver_bit_frm->GetBitIndex(bit_to_flip);
            bit_to_flip->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->InsertPassiveErrorFrame(global_index + 1);
            driver_bit_frm->InsertPassiveErrorFrame(global_index + 1);

            driver_bit_frm->GetBitOf(0, BitType::Intermission)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
            driver_bit_frm->InsertPassiveErrorFrame(1, BitType::Intermission);

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
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            this->dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }

};