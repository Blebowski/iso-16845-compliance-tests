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
 * @test ISO16845 8.1.8
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a frame on reception of an SOF starting at the third bit of the
 *        intermission field following the arbitration winning frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          Intermission field 2 bits, FDF = 0
 *      CAN FD enabled:
 *          Intermission field 2 bits, FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits;
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT sends a frame with higher priority at the same time, to force an
 *  arbitration lost for the frame sent by the IUT. At start of intermission,
 *  the LT waits for 2 bit times before sending an SOF.
 *
 * Response:
 *  The IUT shall repeat the frame starting with the identifier without
 *  transmitting any SOF.
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

class TestIso_8_1_8 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            /* Basic setup for tests where IUT transmits */
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base);

            int gold_ids[] = {
                0x7B,
                0x3B
            };
            int lt_ids[] = {
                0x7A,
                0x3A
            };

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x0, gold_ids[elem_test.index - 1]);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags, 0x0, lt_ids[elem_test.index - 1]);

            driver_bit_frm = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Loose arbitration on last bit of ID.
             *   2. Force last bit of driven frame intermission to dominant. This emulates LT
             *      sending SOF after 2 bits of intermission.
             *   3. Append the same frame to driven and monitored frame. On driven frame, turn
             *      second frame as if received.
             *   4. Remove SOF from 2nd monitored frame.
             *************************************************************************************/
            monitor_bit_frm->LooseArbitration(10, BitType::BaseIdentifier);

            driver_bit_frm->GetBitOf(2, BitType::Intermission)->bit_value_ = BitValue::Dominant;
            
            driver_bit_frm_2->TurnReceivedFrame();

            monitor_bit_frm_2->RemoveBit(0, BitType::Sof);
            driver_bit_frm_2->RemoveBit(0, BitType::Sof);

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

            FreeTestObjects();
            return FinishElementaryTest();
        }

        ENABLE_UNUSED_ARGS
};