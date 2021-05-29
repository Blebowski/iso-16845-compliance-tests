/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 24.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.2.6
 *
 * @brief This test verifies that the IUT detects an acknowledgement error when
 *        the received ACK slot is recessive.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          ACK Slot = 1 bit, FDF = 0
 *      CAN FD enabled:
 *          ACK Slot = 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          #1 ACK slot = recessive
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a base format frame. Then, the LT does
 *  not send the ACK slot according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame starting at the bit position
 *  following the ACK slot.
 *  The IUT shall restart the transmission of the frame as soon as the bus is
 *  idle.
 * 
 * Note:
 *  For classical format frame usage, the IUT shall generate an error frame
 *  starting at the bit position following the 1-bit wide ACK slot.
 *  For FD format frame, the IUT shall generate an error frame starting at the
 *  bit position following the 2-bit wide ACK slot.
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

class TestIso_8_2_6 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::CanFd));

            /* Basic settings where IUT is transmitter */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* Second frame the same due to retransmission. */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);                    
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as received, force ACK slot recessive to emulate missing ACK).
             *   2. Insert expected error frame. In CAN 2.0, expected from ACK Delimiter. In CAN FD
             *      expect from EOF (as if ACK had 2 bits).
             *   3. Append the same frame after the end of first one (to check retransmission)
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::AckDelimiter);
            driver_bit_frm->InsertPassiveErrorFrame(0, BitType::AckDelimiter);

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