/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.11
 * 
 * @brief This test verifies that an error active IUT increases its REC by 8
 *        when detecting a dominant bit as the first bit after sending an error
 *        flag.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 * 
 *  CAN FD Enabled
 *      REC, FDF = 1
 * 
 * Elementary test cases:
 *      #1 Dominant bit at the bit position following the end of the error flag
 *         sent by the IUT.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an active error flag in data field. The
 *  LT sends a dominant bit according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 after reception of the dominant
 *  bit sent by the LT.
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

class TestIso_7_6_11 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, RtrFlag::DataFrame);
            golden_frm = std::make_unique<Frame>(*frame_flags, 1, &error_data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /*************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit! This will
             *      cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Insert Dominant bit on first bit of Error delimiter. This Will shift error
             *      delimiter by 1 bit since DUT shall wait for monitoring Recessive bit!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            /* 
             * Insert Dominant bit before first bit of Error delimiter!
             * On monitor, this bit shall be recessive!
             */
            Bit *bit = driver_bit_frm->GetBitOf(0, BitType::ErrorDelimiter);
            int bit_index = driver_bit_frm->GetBitIndex(bit);

            driver_bit_frm->InsertBit(BitType::ErrorDelimiter, BitValue::Dominant, bit_index);
            monitor_bit_frm->InsertBit(BitType::ErrorDelimiter, BitValue::Recessive, bit_index);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            
            CheckLowerTesterResult();
            CheckNoRxFrame();
            
            /* 1 for original error frame, 8 for dominant bit after error flag */
            CheckRecChange(rec_old, +9);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};