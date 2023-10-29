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
 * @date 11.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.7
 *
 * @brief This test verifies the behaviour of an IUT in protocol exception
 *        state when receiving frames separated by different times for
 *        inter-frame space.
 * @version CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *      CAN FD Tolerant:
 *          Intermission field length
 *
 *      CAN FD enabled:
 *          Intermission field length
 *          Protocol exception handling shall be enabled
 *
 * Elementary test cases:
 *   #1 The second frame starts after the third intermission bit + 1 bit time
 *      after the first frame.
 *   #2 The second frame starts after the third intermission bit of the first
 *      frame.
 *   #3 The second frame starts after the second intermission bit of the first
 *      frame followed by a third frame starts after the third intermission bit
 *      of the previous frame.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT send a frame with non-nominal bit in control field causing protocol
 *  exception behaviour.
 *  The LT send a valid classical frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall only acknowledge the last test frame in each test sequence.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_7 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::FdTolerantFdEnabled);
            for (int i = 0; i < 3; i++)
                AddElemTest(test_variants[0], ElementaryTest(i + 1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
            dut_ifc->ConfigureProtocolException(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            frame_flags_2 = std::make_unique<FrameFlags>(FrameKind::Can20);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2);
            RandomizeAndPrint(golden_frm_2.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Flip bit which shall cause protocol exception to dominant! This is needed in
             *      CAN FD Enabled variant only, since in CAN FD Tolerant variant FDF bit is
             *      sufficient to invoke protocol exception!
             *   2. Update frame since CRC might have changed. This should not have impact since
             *      IUT is going to protocol exception!
             *   3. Turn monitored frame as if received. Force ACK to recessive since IUT is in
             *      protocol exception!
             *   4. Modify end of intermission as per elementary test. Add one bit of idle in first
             *      elementary test case, remove one bit in last test.
             *   5. Append second frame send by IUT. In third elementary test, IUT shall not ack
             *      this frame. Force monitored ACK to recessive.
             *   6. For last elementary test, append one more frame!
             *************************************************************************************/
            if (test_variant == TestVariant::CanFdEnabled)
            {
                driver_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
                monitor_bit_frm->GetBitOf(0, BitKind::R0)->val_ = BitVal::Recessive;
            }

            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            monitor_bit_frm->ConvRXFrame();
            monitor_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            /*
             * I think LT must send dominant ACK bit in the first frame although it is not
             * explicitly stated in ISO11898-1 2016! Only that-way it is able to distuiguish
             * duration of Integration, because the situation after first frame is:
             *   1. Elem. test - ACK Delim + 7 EOF bits + 3 intermission + 1 extra = 12 bits
             *                   IUT should have reintegrated, therefore ACK second frame.
             *   2. Elem. test - ACK Delim + 7 EOF bits + 3 intermission = 11 bits.
             *                   IUT should have reintegrated, therefore ACK second frame.
             *   3. Elem. test - ACK Delim + 7 EOF bits + 2 intermission = 10 bits.
             *                   IUT should not have managed to reintegrate, therefore it
             *                   shall not ACK second frame, but only third frame (which has
             *                   11 recessive bits since second one).
             *
             * Also CAN FD frames in model have two ACK bits therefore to get the sequence right
             * (only 10 consecutive recessive in third elementary test), also second ACK bit must
             * be forced to dominant!
             *
             * TODO: Make sure that standard meant it thisway!
             */
            driver_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            driver_bit_frm->GetBitOf(1, BitKind::Ack)->val_ = BitVal::Dominant;

            if (elem_test.index_ == 1)
            {
                driver_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                monitor_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
            } else if (elem_test.index_ == 3) {
                driver_bit_frm->RemoveBit(2, BitKind::Interm);
                monitor_bit_frm->RemoveBit(2, BitKind::Interm);
            }

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2->ConvRXFrame();
            if (elem_test.index_ == 3)
                monitor_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            if (elem_test.index_ == 3)
            {
                driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
                monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
                monitor_bit_frm_2->ConvRXFrame();
                driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());
            }

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm_2);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};