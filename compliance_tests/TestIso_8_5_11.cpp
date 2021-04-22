/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 3.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.5.11
 * 
 * @brief The purpose of this test is to verify that an IUT which is bus-off
 *        is not permitted to become error active (no longer bus-off) before
 *        128 occurrences of 11 consecutive recessive bits.
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
 *      1) the LT sends recessive bus level for at least 1 408 bit times until
 *         the IUT becomes active again;
 *      2) the LT sends one group of 10 recessive bits, one group of 21 recessive
 *         bits followed by at least 127 groups of 11 recessive bits, each group
 *         separated by 1 dominant bit.
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT ask the IUT to send a frame and sets it in the bus-off state.
 * 
 *  The LT sends profiles defined in elementary test cases.
 *  
 * Response:
 *  The IUT shall not transmit the frame before the end of the profiles sent by
 *  the LT according to elementary test cases and shall send it before the end
 *  of the TIMEOUT.
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

class TestIso_8_5_11 : public test_lib::TestBase
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

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* First frame */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, BrsFlag::DontShift,
                                                       EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frames as received. Force ACK Delimiter low. This will cause form
             *      error at transmitter and unit to become bus-off.
             *   2. Insert Passive Error frame from next bit on to both driven and monitored
             *      frames.
             *   3. Append test sequences as given by elementary test.
             *      
             *   Note: This does not check that frame will be retransmitted before timeout!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::AckDelimiter)->bit_value_ = BitValue::Dominant;

            Bit *eof_bit = driver_bit_frm->GetBitOf(0, BitType::Eof);
            int eof_start = driver_bit_frm->GetBitIndex(eof_bit);

            driver_bit_frm->InsertPassiveErrorFrame(eof_start);
            monitor_bit_frm->InsertPassiveErrorFrame(eof_start);

            int interm_index = driver_bit_frm->GetBitIndex(
                                driver_bit_frm->GetBitOf(0, BitType::Intermission));
            driver_bit_frm->RemoveBitsFrom(interm_index);
            monitor_bit_frm->RemoveBitsFrom(interm_index);

            if (elem_test.index == 1)
            {
                for (int i = 0; i < 1408; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }

                // IUT specific compensation
                //for (int i = 0; i < 14; i++)
                //{
                //    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                //    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                //}

            } else {

                for (int i = 0; i < 10; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                driver_bit_frm->AppendBit(BitType::Idle, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);

                for (int i = 0; i < 21; i++)
                {
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                driver_bit_frm->AppendBit(BitType::Idle, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);

                for (int i = 0; i < 127; i++)
                {
                    for (int j = 0; j < 11; j++)
                    {
                        driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                        monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                    }
                    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Dominant);
                    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                }
                driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);

                // IUT specific compensation
                //for (int i = 0; i < 11; i++)
                //{
                //    driver_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                //    monitor_bit_frm->AppendBit(BitType::Idle, BitValue::Recessive);
                //}
            }

            //driver_bit_frm_2->TurnReceivedFrame();
            //driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            //monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /***************************************************************************** 
             * Execute test
             *****************************************************************************/
            dut_ifc->SetTec(255); /* just before bus-off */
            dut_ifc->SendReintegrationRequest(); /* Request in advance, DUT will hold it */
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            /* Must restart DUT for next iteration since it is bus off! */
            this->dut_ifc->Disable();
            this->dut_ifc->Reset();
            this->dut_ifc->Enable();

            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(2000);

            return FinishElementaryTest();
        }

};