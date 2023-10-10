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
 * @test ISO16845 7.6.4
 * 
 * @brief This test verifies that the IUT increases its REC by 8 when detecting
 *        the eighth consecutive dominant bit following the transmission of its
 *        overload flag and after each sequence of additional 8 consecutive
 *        dominant bits.
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
 *      #1 16 bit dominant
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to generate an overload frame after a data frame.
 *  After the overload flag sent by the IUT, the LT sends a sequence of dominant
 *  bits according to elementary test cases.
 * 
 * Response:
 *  The IUT’s REC value shall be increased by 8 on each eighth dominant bit
 *  after the overload flag.
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

class TestIso_7_6_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force last bit of EOF to Dominant!
             *   3. Insert Overload frame from first bit of Intermission.
             *   4. Insert 16 Dominant bits directly after Overload frame (from first bit
             *      of Overload Delimiter). These bits shall be driven on can_tx, but 16
             *      RECESSIVE bits shall be monitored on can_tx. 
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->InsertOverloadFrame(0, BitType::Intermission);
            driver_bit_frm->InsertOverloadFrame(0, BitType::Intermission);

            Bit *ovr_delim = driver_bit_frm->GetBitOf(0, BitType::OverloadDelimiter);
            int bit_index = driver_bit_frm->GetBitIndex(ovr_delim);

            for (int k = 0; k < 16; k++)
            {
                driver_bit_frm->InsertBit(BitType::OverloadFlag, BitValue::Dominant, bit_index);
                monitor_bit_frm->InsertBit(BitType::OverloadFlag, BitValue::Recessive, bit_index);
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

            /*
             * Receiver will make received frame valid on 6th bit of EOF! Therefore at
             * point where Error occurs, frame was already received OK and should be
             * readable!
             */
            CheckRxFrame(*golden_frm);
            
            /* 
             * For first iteration we start from 0 so there will be no decrement on
             * sucessfull reception! For further increments there will be alway decrement
             * by 1 and increment by 2*8.
             */
            if (test_variant == TestVariant::Common)
                CheckRecChange(rec_old, +16);
            else
                CheckRecChange(rec_old, +15);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};