/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.3.3
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a bit
 *        error when one of the 6 dominant bits of the error flag it transmits
 *        is forced to recessive state by LT.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          FDF = 0
 *      CAN FD Enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      Elementary tests to perform:
 *          #1 corrupting the first bit of the error flag;
 *          #2 corrupting the fourth bit of the error flag;
 *          #3 corrupting the sixth bit of the error flag.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT corrupts this frame in data field causing the IUT to send an active
 *  error frame.
 *  Then the LT forces one of the 6 bits of the active error flag sent by the
 *  IUT to recessive state according to elementary test cases.
 * 
 * Response:
 *  The IUT shall restart its active error flag at the bit position following
 *  the corrupted bit.
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

class TestIso_8_3_3 : public test_lib::TestBase
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

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            
            // Note: In this test we cant enable TX/RX feedback, since we want to corrupt DOMINANT
            //       active error flag! This is not possible when DUT transmitts dominant. We
            //       therefore disable it and compensate for what DUT is receiving, so that DUT
            //       will not see bit errors!
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80; // 7-th data bit will be recessive stuff bit
            if (test_variant == TestVariant::Common)
                frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, RtrFlag::DataFrame);
            else
                frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, EsiFlag::ErrorActive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Flip 7-th data bit of driven frame to dominant, this will destroy recessive
             *     stuff bit send by IUT.
             *  2. Insert expected active error frame from 8-th bit of data field to monitored
             *     frame. Insert the same to driven frame.
             *  3. Flip 1,4 or 6-th bit of Error flag to Recessive. Insert next expected error
             *     frame from one bit further.
             *  4. Turn second driven frame (the same) as received. Append after first frame. This
             *     checks retransmission.
             * 
             *  Note: TX/RX feedback is disabled, we need to drive the same what we monitor so that
             *        IUT will see its own frame!
             *************************************************************************************/
            driver_bit_frm->GetBitOf(6, BitType::Data)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            int bit_index_to_flip;
            if (elem_test.index == 1)
                bit_index_to_flip = 1;
            else if (elem_test.index == 2)
                bit_index_to_flip = 4;
            else if (elem_test.index == 3)
                bit_index_to_flip = 6;

            Bit *bit_to_flip = driver_bit_frm->GetBitOf(bit_index_to_flip - 1,
                                                        BitType::ActiveErrorFlag);
            bit_to_flip->bit_value_ = BitValue::Recessive;
            int next_err_flg_index = driver_bit_frm->GetBitIndex(bit_to_flip) + 1;

            driver_bit_frm->InsertActiveErrorFrame(next_err_flg_index);
            monitor_bit_frm->InsertActiveErrorFrame(next_err_flg_index);

            // Append next frame. Needs to have ACK set!
            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
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