/*****************************************************************************
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
 * @date 14.2.2021
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.8.2.4
 *
 * @brief The purpose of this test is to verify the secondary sample point of
 *        an IUT acting as a transmitter with a delay, d. The test shall be
 *        applied before the sample-point of the transmitter’s CRC delimiter.
 * @version CAN FD enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Available configuration methods for delay compensation = fix programmed
 *      or automatically measured.
 *          Delay, d, in range of TQ (D) = d ∈ (1, 2 data bit times)
 *          FDF = 1
 *
 * Elementary test cases:
 *  There are two elementary tests to perform for 1 bit rate configuration and
 *  each way of configuration of delay compensation - fix programmed or
 *  automatically measured, shall be checked.
 *      #1 d = 1 data bit times
 *      #2 d = 2 data bit times
 *
 *  — Check sampling point by applying the correct bit value only at programmed
 *    position of secondary sampling point.
 *
 *  Each available way of configuration of delay compensation, shall be checked
 *  separately by execution of test #1 to #2.
 *
 *  Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation shall be enabled. SSP offset shall be confi-
 *  gured to evaluate the delayed bit on similar position like the sampling
 *  point in data phase [Sampling_Point(D)].
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT prolonged the SOF bit on IUT receive input by an amount of d
 *  according to elementary to elementary test cases to shift the IUT received
 *  sequence relative against the transmitted sequence of IUT.
 *
 *  The LT disturbs a bit on IUT receive input line at bit position of last
 *  transmitted CRC bit by inverting the bit value.
 *
 *  In this disturbed bit, the LT inserts a pulse of 2 TQ(D) around the
 *  secondary sampling point of correct bit value.
 *  Start of correct value at: delay compensation + offset − 1TQ(D) relative
 *  to transmitted bit.
 *
 * Response:
 *  The modified CRC bit shall be sampled as its nominal value.
 *  The frame is valid. No error flag shall occur.
 *
 * Note:
 *  The bit disturbance of IUT receive line will start at that bit on the
 *  receive bit stream which occur right on time before CRC delimiter will be
 *  send by IUT on transmit line so that the expected SSP occurs before CRC
 *  delimiter starts.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_2_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);

            /*
             * Test defines only two elementary tests, but each type of SSP shall be tested.
             * We have options: Offset, Offset + Measured. This gives us two options for each
             * elementary test, together 4 tests.
             */
            for (size_t i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1));

            SetupMonitorTxTests();

            // Following constraint is not due to model or IUT issues.
            // It is due to principle of the test, we can't avoid it!
            // This is because we are delaying received sequence by up to: 2 x Bit time (D).
            // If such big delay is applied, and TSEG1(N) is smaller than this number, an
            // error frame is detected still in Nominal Bit-rate.
            TEST_ASSERT(dbt.GetBitLenCycles() * 2 < ((nbt.ph1_ + nbt.prop_ + 1) * nbt.brp_),
                        " In this test TSEG1(N) > 2 * Bit time(D) due to test architecture!");

            TEST_ASSERT(dbt.GetBitLenCycles() * 3 < dut_max_secondary_sample,
                        "Bit time (N) * 3 < Limit for maximal Secondary sample point compensation!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, RtrFlag::Data,
                                                       BrsFlag::DoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Delay received sequence by d data bit times:
             *          Elem test 1,2 : d = 1
             *          Elem test 3,4 : d = 2
             *      This is done by prolonging SOF of driven frame.
             *   2. Force last bit of CRC to opposite value.
             *   3. Insert 2 TQ pulse of correct value around sample point of last bit of CRC.
             *   4. Insert ACK so that frame is correctly transmitted.
             *************************************************************************************/
            size_t d = dbt.GetBitLenCycles();
            if (elem_test.index_ == 3 || elem_test.index_ == 4)
                d *= 2;
            drv_bit_frm->GetBit(0)->GetTQ(0)->Lengthen(d);

            auto bit_it = drv_bit_frm->GetBitOfIter(0, BitKind::CrcDelim);
            bit_it--;
            BitVal correct_bit_value = bit_it->val_;
            bit_it->FlipVal();

            auto tq_it = bit_it->GetLastTQIter(BitPhase::Ph1);
            // Insert pulse around point which is 2 TQ before sample point.
            tq_it--;
            tq_it->ForceVal(correct_bit_value);
            tq_it--;
            tq_it->ForceVal(correct_bit_value);

            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/

            /* Reconfigure SSP: Test 1, 3 -> Measured + Offset, Test 2, 4 -> Offset only */
            dut_ifc->Disable();
            if (elem_test.index_ == 1 || elem_test.index_ == 3)
            {
                /* Offset as if two time quantas before regular sample point! This is because if
                 * we set offset as if in sample point, the SSP for last bit of CRC would already
                 * be ignored since it reaches to SP of CRC delimiter. Test description explicitly
                 * says that SSP shall be configured before SP of CRC delimiter. Since prolonging
                 * of SSP past CRC delimiter SP is optional, we must set it just before SP of CRC
                 * Delimiter to properly test this feauter!or
                 *
                 * TX/RX delay will be measured and added by IUT. Offset in clock cycles!
                 * (minimal time quanta)
                 */
                size_t ssp_offset = dbt.brp_ * (dbt.prop_ + dbt.ph1_ - 1);
                dut_ifc->ConfigureSsp(SspType::MeasAndOffset, static_cast<int>(ssp_offset));
            } else {
                /* We need to incorporate d into the delay! Also, move offest slightly before
                 * regular sample point so that last bit is not lost due to already disabled
                 * SSP at CRC delimiter!
                 */
                size_t ssp_offset = dbt.brp_ * (dbt.prop_ + dbt.ph1_ - 1) + d;
                dut_ifc->ConfigureSsp(SspType::Offset, static_cast<int>(ssp_offset));
            }
            dut_ifc->Enable();
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(2000);

            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            FreeTestObjects();
            return FinishElemTest();
        }
};