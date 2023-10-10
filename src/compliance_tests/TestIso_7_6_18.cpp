/****************************************************************************** 
 * 
 * ISO16845 Compliance tests 
 * Copyright (C) 2021-present Ondrej Ille
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 * 
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 16.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.18
 * 
 * @brief This test verifies that the IUT does not increase its REC after the
 *        seventh bit of the received error flag.
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
 *      #1 7 dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an active error frame in data field.
 *  After the error flag sent by the IUT, the LT sends a sequence according to
 *  elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be not further incremented after the increment
 *  due to the dominant bit which followed the error flag sent by the IUT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "pli_lib.h"

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

class TestIso_7_6_18 : public test_lib::TestBase
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
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, RtrFlag::DataFrame);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &error_data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force 7-th bit of Data frame to opposite, this should be stuff bit! This will
             *      cause stuff error!
             *   3. Insert Active Error frame from 8-th bit of data frame!
             *   4. Insert 7 Dominant bits directly after Error frame (from first bit of Error
             *      Delimiter). These bits shall be driven on can_tx, but 7 RECESSIVE bits shall
             *      be monitored on can_tx.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);
            driver_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            Bit *err_delim = driver_bit_frm->GetBitOf(0, BitType::ErrorDelimiter);
            int bit_index = driver_bit_frm->GetBitIndex(err_delim);

            for (int k = 0; k < 7; k++)
            {
                driver_bit_frm->InsertBit(BitType::ActiveErrorFlag, BitValue::Dominant, bit_index);
                monitor_bit_frm->InsertBit(BitType::ActiveErrorFlag, BitValue::Recessive, bit_index);
            }

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

            /* 
             * Check that REC has incremented only by 9 (due to first error frame +
             * detection of dominant bit as first bit post error flag)
             */
            CheckRecChange(rec_old, +9);
            
            FreeTestObjects();
            return FinishElementaryTest();
        }
};