/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 2.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.5.10
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
 *      #1 The LT forces the bus to recessive for bus-off recovery time
 *         (22 bits).
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame twice.
 *  The LT causes the IUT to generate an error frame. During the error flag
 *  transmitted by the IUT, the LT forces recessive state during 16 bit times.
 *  After the following passive error flag, the error delimiter is forced to
 *  dominant state for 112 bit times.
 * 
 *  Then, the IUT transmits its first frame. The LT acknowledges the frame
 *  and immediately causes the IUT to generate an overload frame.
 *
 *  The LT forces the first bit of this overload flag to recessive state
 *  creating a bit error. (6 + 7) bit times later, the LT generates a dominant
 *  bit to cause the IUT to generate a new overload frame.
 * 
 *  The LT forces the first bit of this new overload flag to recessive state
 *  causing the IUT to increments its TEC to the bus-off limit.
 * 
 *  (6 + 8 + 3 + 8) bit times later, the LT sends a valid frame according to
 *  elementary test cases.
 *  
 * Response:
 *  Only one frame shall be transmitted by the IUT.
 *  The IUT shall not acknowledge the frame sent by the LT.
 *  Error counter shall be reset after bus-off recovery.
 * 
 * Note:
 *  Check error counter after bus-off, if applicable.
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

class TestIso_8_5_10 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 1;
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            /* First frame */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                            IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                            EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x8, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /* Second frame */
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type,
                                                            RtrFlag::DataFrame);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2);
            RandomizeAndPrint(golden_frm_2.get());

            /* At first, frm_2 holds the same retransmitted frame! */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);                    
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force 7-th data bit to dominant to cause stuff error.
             *   2. Insert 16 dominant bits from next bit of monitored frame. Insert 16 recessive
             *      bits from next bit of driven frame. This emulates DUT always re-starting error
             *      detecting bit error during active error flag.
             *   3. Insert 6 recessive bits to emulate passive error flag (both driven and
             *      monitored frames).
             *   4. Insert 112 dominant bits to driven frame and 112 recessive bits to monitored
             *      frame. Then Insert 8 + 3 + 8 (Error delimiter + intermission + Suspend) to
             *      recessive frames.
             *   5. Insert second frame as if transmitted by DUT. Append the same frame on driven
             *      frame since TX/RX feedback is disabled! This is the same frame as before
             *      because it is retransmitted by DUT!
             *   6. Force first bit of intermission to dominant state -> Overload condition. This
             *      is in fact 4th intermission bit (overall since there were) three bits before!
             *   7. Insert one dominant bit on monitored frame and one recessive bit on driven
             *      frame. This emulates expected first bit of overload flag and corruption of its
             *      first bit.
             *   8. Insert 6+7 recessive bits on both driven and monitored frames. This  emulates
             *      Passive error flag and error delimiter.
             *   9. Insert one dominant bit to driven frame, and one recessive bit to monitored
             *      frame. This represents next overload condition.
             *  10. Insert 1 recessive bit to driven frame (error on first bit of overload frame).
             *      Insert 1 dominant bit to monitored frame (first bit) of overload frame. This
             *      should cause DUT to go Bus-off.
             *  11. Insert 6 + 8 + 3 + 8 recessive bits to both driven and monitored frames. This
             *      corresponds to Passive Error flag, Error delimiter + Intermission + possible
             *      suspend.
             *  12. Insert third frame as if sent by LT. In driven frame, this frame is as if
             *      transmitted. In monitored frame, it is all recessive (including ACK) since IUT
             *      shall be bus-off.
             **************************************************************************************/
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            int index_to_remove = driver_bit_frm->GetBitIndex(
                                    driver_bit_frm->GetBitOf(7, BitType::Data));
            driver_bit_frm->RemoveBitsFrom(index_to_remove);
            monitor_bit_frm->RemoveBitsFrom(index_to_remove);

            for (int i = 0; i < 16; i++)
            {
                driver_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Dominant);
            }

            for (int i = 0; i < 6; i++)
            {
                driver_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
            }

            for (int i = 0; i < 112; i++)
            {
                driver_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::ActiveErrorFlag, BitValue::Recessive);
            }

            for (int i = 0; i < 8; i++)
            {
                driver_bit_frm->AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);
            }
            
            for (int i = 0; i < 3; i++)
            {
                driver_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);
            }

            for (int i = 0; i < 8; i++)
            {
                driver_bit_frm->AppendBit(BitType::Suspend, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::Suspend, BitValue::Recessive);
            }

            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            /* Compensate ESI of second frame in second elementary test. Then IUT is already
             * passive!
             */
            if (test_variant == TestVariant::CanFdEnabled)
            {
                monitor_bit_frm_2->GetBitOf(0, BitType::Esi)->bit_value_ = BitValue::Recessive;
                driver_bit_frm_2->GetBitOf(0, BitType::Esi)->bit_value_ = BitValue::Recessive;

                monitor_bit_frm_2->UpdateFrame();
                driver_bit_frm_2->UpdateFrame();
            }

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            /* Actuall first bit of intermission after second frame */
            driver_bit_frm->GetBitOf(3, BitType::Intermission)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->GetBitOf(4, BitType::Intermission)->bit_value_ = BitValue::Recessive;
            monitor_bit_frm->GetBitOf(4, BitType::Intermission)->bit_value_ = BitValue::Dominant;

            /* Remove last bit of intermission */
            driver_bit_frm->RemoveBit(driver_bit_frm->GetBitOf(5, BitType::Intermission));
            monitor_bit_frm->RemoveBit(monitor_bit_frm->GetBitOf(5, BitType::Intermission));

            for (int i = 0; i < 6; i++)
            {
                driver_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
            }
            for (int i = 0; i < 7; i++)
            {
                driver_bit_frm->AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);
            }

            /* Next overload condition */
            driver_bit_frm->AppendBit(BitType::Intermission, BitValue::Dominant);
            monitor_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);

            /* Error on first bit of overload flag */
            driver_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);
            monitor_bit_frm->AppendBit(BitType::Intermission, BitValue::Dominant);

            /* 6 + 8 + 8 + 3 frames */
            for (int i = 0; i < 25; i++)
            {
                /* Bit type in frame is don't care pretty much */
                driver_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);
            }

            /* Append as if third frame which DUT shall not ACK (its bux off) */
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            
            monitor_bit_frm_2->TurnReceivedFrame();
            monitor_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;
            
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            
            CheckLowerTesterResult();

            /* Must restart DUT for next iteration since it is bus off! */
            this->dut_ifc->Disable();
            this->dut_ifc->Enable();
            
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(2000);

            return FinishElementaryTest();
        }

        ENABLE_UNUSED_ARGS
};