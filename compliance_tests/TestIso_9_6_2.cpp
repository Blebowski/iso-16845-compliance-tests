/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.2.2021
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 9.6.2
 * 
 * @brief This test verifies that increasing REC and TEC are independent operations.
 * @version CAN FD enabled
 * 
 * Test variables:
 *     Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          REC
 *          TEC
 *          FDF = 0
 * 
 *     CAN FD Enabled:
 *          REC
 *          TEC
 *          FDF = 1
 * 
 * Elementary test cases:
 *  There is one elementary test to perform. 
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to increase its TEC up to 127. Then, LT causes the
 *  IUT to increase its REC up to 128. Then, the LT sends a frame containing
 *  a stuff error in data field.
 * 
 * Response:
 *  Each increment of the REC shall be responded by an active error flag.
 *  The IUT responds to the stuff error with a passive error flag.
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

class TestIso_9_6_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);

            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            // This test has IUT as receiver, so no trigger/waiting config is needed!
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorActive);
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0xAA, &data_byte);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, 0x1, 0xAA, &data_byte);
            golden_frm->Print();

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            // Separate frame is needed for CAN FD enabled variant. This frame is already with
            // IUT being Error passive, so we need frame/frame_flags with ESI error passive!
            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip monitored frame as if received.
             *   2. Flip 7-th bit of data field. This should bit stuff bit. Do this in both frames.
             *   3. Insert Active Error frame to Monitored frame from next bit. Insert Active
             *      Error frame to driven frame (TX/RX feedback disabled). Do this in both frames.
             *      
             *      This first frame will be transmitted 128 times. This accounts for incrementing
             *      REC to 128.  
             * 
             *   4. In second frame, turn monitored frame as if received.
             *   5. In driven frame, flip 7-th bit of data field to cause stuff error.
             *   6. Insert Passive Error frame to both driven and monitored frames.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            monitor_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm_2->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm_2->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm_2->InsertPassiveErrorFrame(7, BitType::Data);


            TestMessage("First frame");
            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            TestMessage("Second frame");
            driver_bit_frm_2->Print(true);
            monitor_bit_frm_2->Print(true);

            /**************************************************************************************
             * Execute test
             *  1. Preset TEC to 127.
             *  2. Try to send frame with error in it 128 times by LT. This should increment
             *     IUTs REC to 128 and IUT will just become error passive.
             *  3. Send one more frame with stuff error in it. This-one should have passive error
             *     frame as response.
             *************************************************************************************/
            dut_ifc->SetTec(127);
            dut_ifc->SetRec(0);

            for (int i = 0; i < 128; i++)
            {
                TestMessage("Sending frame nr. : %d", i);
                int rec_old = dut_ifc->GetRec();
                PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                RunLowerTester(true, true);
                CheckLowerTesterResult();
                CheckRecChange(rec_old, +1);
            }

            TestMessage("Sending frame which should lead to passive error flag!");
            PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};