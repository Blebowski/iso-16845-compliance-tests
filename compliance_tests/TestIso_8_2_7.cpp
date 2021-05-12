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
 * @test ISO16845 8.2.7
 *
 * @brief This test verifies the behaviour in the CRC delimiter and acknowledge
 *        field when these fields are extended to 2 bits.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      CRC delimiter
 *      ACK slot
 *      FDF = 1
 *
 * Elementary test cases:
 *      There are two elementary tests to perform:
 *          #1 CRC delimiter up to 2-bit long (late ACK bit – long distance);
 *          #2 ACK up to 2-bit long (superposing ACK bits – near and long distance).
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT creates a CRC
 *  delimiter and an ACK bit as defined in elementary test cases.
 *
 * Response:
 *  The frame is valid. The IUT shall not generate an error frame.
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

class TestIso_8_2_7 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (int i = 0; i < 2; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            /* Basic settings where IUT is transmitter */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as received.
             *   2. In elementary test 1 (2 bit CRC delimiter):
             *          Flip first ACK bit to Recessive. Flip second ACK bit to Dominant.
             *      In elementary test 2 (2 bit ACK):
             *          Flip both ACK bits to Dominant.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            if (elem_test.index == 1)
                driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;
            else
                driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
            
            driver_bit_frm->GetBitOf(1, BitType::Ack)->bit_value_ = BitValue::Dominant;

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