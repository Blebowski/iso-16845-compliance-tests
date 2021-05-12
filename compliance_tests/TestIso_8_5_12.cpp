/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 31.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.12
 *
 * @brief The purpose of this test is to verify that a passive state IUT acting
 *        as a transmitter waits for 6 consecutive identical bit to complete
 *        its passive error flag.
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
 *      There is one elementary test to perform.
 *          #1 During the error flag, the LT sends 5 dominant bits,
 *             5 recessive bits and then, 6 dominant bits.
 * 
 * Setup:
 *  The IUT is set to the TEC passive state
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT corrupts a bit in data field to cause the IUT to generate a
 *  passive error flag according to elementary test cases.
 *  After the 6 dominant bits, the LT waits for 8 bit time before sending a
 *  dominant bit.
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

class TestIso_8_5_12 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));            
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            /* Basic settings where IUT is transmitter */
            SetupMonitorTxTests();
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
             *   2. Insert 5 dominant, 5 recessive and 6 dominant bits to driven frame right after
             *      bit with stuff error. Cut trailing bits. In monitored frame, insert 16
             *      recessive bits.
             *   3. Append 8 recessive bits to both driven and monitored frames.
             *   4. Insert single dominant bit to driven frame and recessive bit to monitored frame.
             *   5. Insert Overload frame to monitored frame after 8-th recessive bit. Insert
             *      Passive error frame to driven frame (TX/RX feedback enabled, just placeholder)
             *   6. Append retransmitted frame by IUT.
             *************************************************************************************/
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->RemoveBitsFrom(7, BitType::Data);
            monitor_bit_frm->RemoveBitsFrom(7, BitType::Data);

            for (int i = 0; i < 5; i++)
            {
                driver_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
            }

            // Compensate IUTs resynchronisation on first dominant bit caused by its input delay.
            monitor_bit_frm->GetBitOf(0, BitType::PassiveErrorFlag)
                ->GetLastTimeQuantaIterator(BitPhase::Ph2)->Lengthen(dut_input_delay);

            for (int i = 0; i < 5; i++)
            {
                driver_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
            }

            for (int i = 0; i < 6; i++)
            {
                driver_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Dominant);
                monitor_bit_frm->AppendBit(BitType::PassiveErrorFlag, BitValue::Recessive);
            }

            for (int i = 0; i < 8; i++)
            {
                driver_bit_frm->AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);
                monitor_bit_frm->AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);
            }

            driver_bit_frm->AppendBit(BitType::Intermission, BitValue::Dominant);
            monitor_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);

            /* 
             * Following bit is inserted just to be immediately overwritten by overload
             * frame!
             */
            driver_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);
            monitor_bit_frm->AppendBit(BitType::Intermission, BitValue::Recessive);

            driver_bit_frm->InsertOverloadFrame(1, BitType::Intermission);
            monitor_bit_frm->InsertOverloadFrame(1, BitType::Intermission);

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
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            this->dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }

};