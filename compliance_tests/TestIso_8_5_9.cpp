/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 29.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.5.9
 * 
 * @brief The purpose of this test is to verify that a passive state IUT does
 *        not transmit a frame starting with an identifier and without
 *        transmitting SOF when detecting a dominant bit on the third bit of
 *        the intermission field.
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
 *      #1 dominant bit on the third bit of the intermission field;
 *      #2 dominant bit on the first bit of Suspend transmission;
 *      #3 dominant bit on the seventh bit of Suspend transmission.
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT corrupts this frame and corrupts the following error frame to set
 *  the IUT into passive state.
 *  The error frame shall end with error delimiter followed by the inter-
 *  mission field.
 *  At the bit, as specified in elementary test cases after the error delimiter,
 *  the LT starts sending a frame with lower priority
 * 
 * Response:
 *  The IUT shall not start the re-transmission before the end of the frame
 *  sent by the LT.
 *  The IUT shall acknowledge the frame sent by the LT.
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

class TestIso_8_5_9 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 3;
            for (int i = 0; i < num_elem_tests; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }            

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            /* First frame */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, RtrFlag::DataFrame);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* Second frame */
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2);
            RandomizeAndPrint(golden_frm_2.get());

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received.
             *   2. Force 7-th data bit to dominant to cause stuff error.
             *   3. Insert Passive Error frame from next bit to monitored frame. Insert Passive
             *      Error frame to driven frame.
             *   4. Modify frames according to elementary test cases:
             *       First elementary test:
             *         Remove last bit of Intermission in both driven and monitored frame.
             *       Second elementary test:
             *         Do nothing, appending after third bit of intermission corresponds
             *         to detecting dominant by IUT during first bit of its suspend field.
             *       Third elementary test:
             *         Append 6 bits of Suspend transmission. If then frame is appended in
             *         next step, this corresponds to detecting dominant at 7-th bit of
             *         suspend field!
             *   5. Append next frame. On monitor as is received by IUT, on driver as if sent by
             *      LT. This checks that IUT becomes receiver of this frame!
             *   6. Append the same frame as first frame once again (after this second frame).
             *      This checks that DUT re-transmitts the first frame which had error on 7-th data
             *      bit. Frame is as if received on driven frame.
             *   7. Append one more intermission, since DUT will succesfully retransmitt the frame
             *      and therefore go to suspend! This is needed to separate it from next time step!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            switch (elem_test.index)
            {
                case 1:
                    driver_bit_frm->RemoveBit(
                        driver_bit_frm->GetBitOf(2, BitType::Intermission));
                    monitor_bit_frm->RemoveBit(
                        monitor_bit_frm->GetBitOf(2, BitType::Intermission));
                    break;
                case 3:
                    for (int i = 0; i < 7; i++)
                    {
                        driver_bit_frm->AppendBit(BitType::Suspend, BitValue::Recessive);
                        monitor_bit_frm->AppendBit(BitType::Suspend, BitValue::Recessive);
                    }
                    break;
                default:
                    break;
            }

            /* Append second one */
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm_2->TurnReceivedFrame();
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            /* Append third one */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);
            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            /* Append Suspend transmission */
            driver_bit_frm->AppendSuspendTransmission();
            monitor_bit_frm->AppendSuspendTransmission();

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /***************************************************************************** 
             * Execute test
             *****************************************************************************/
            dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm_2);

            return FinishElementaryTest();
        }

        ENABLE_UNUSED_ARGS
};