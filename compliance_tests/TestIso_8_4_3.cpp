/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 9.8.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.3
 *
 * @brief The purpose of this test is to verify that an IUT is able to transmit
 *        a data frame starting with the identifier and without transmitting
 *        SOF, when detecting a dominant bit on the third bit of the intermi-
 *        ssion field following an overload frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 0
 *      CAN FD Enabled:
 *          Intermission field = 2 bit
 *          FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 * 
 *      There are two elementary tests to perform:
 *          #1 the identifier shall start with 4 dominant bits.
 *          #2 the identifier shall start with 5 recessive bits.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *  The LT disturbs the transmitted frame with an error frame, then the LT causes
 *  the IUT to generate an overload frame immediately after the error frame.
 *  Then, the LT forces the third bit of the intermission following the overload
 *  delimiter to dominant state.
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

class TestIso_8_4_3 : public test_lib::TestBase
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

            /* Standard settings for tests where IUT is transmitter */
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            int ids[] = {
                0x7B,
                0x3B
            };

            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, ids[elem_test.index - 1],
                                                 &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Turn driven frame as received.
             *  2. Force 7-th data bit to Dominant. This should be recessive stuff bit. Insert
             *     Active Error frame from next bit on, to monitored frame. Insert Passive Error
             *     frame to driven frame.
             *  3. Force 8-th bit of Error delimiter to dominant. Insert Overload frame from next
             *     bit on to monitored frame. Insert Passive Error frame to driven frame.
             *  4. Force third bit of intermission after overload frame to dominant (in driven
             *     frame).
             *  5. Remove SOF bit from retransmitted frame. Append retransmitted frame behind the
             *     first frame. Second driven frame is turned received.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();
            
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            Bit *last_err_delim_bit = driver_bit_frm->GetBitOf(7, BitType::ErrorDelimiter);
            int last_err_delim_index = driver_bit_frm->GetBitIndex(last_err_delim_bit);
            
            last_err_delim_bit->bit_value_ = BitValue::Dominant;
            monitor_bit_frm->InsertOverloadFrame(last_err_delim_index + 1);
            driver_bit_frm->InsertPassiveErrorFrame(last_err_delim_index + 1);

            Bit *third_intermission_bit = driver_bit_frm->GetBitOf(2, BitType::Intermission);
            third_intermission_bit->bit_value_ = BitValue::Dominant;

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm_2->RemoveBit(driver_bit_frm_2->GetBitOf(0, BitType::Sof));
            monitor_bit_frm_2->RemoveBit(monitor_bit_frm_2->GetBitOf(0, BitType::Sof));

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