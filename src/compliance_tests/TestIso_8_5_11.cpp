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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_5_11 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 2; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {

            /* First frame */
            frm_flags = std::make_unique<FrameFlags>(
                elem_test.frame_kind_, IdentKind::Base, RtrFlag::Data,
                BrsFlag::NoShift, EsiFlag::ErrPas);

            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            // In FD enabled variant, the retransmitted frame will be in error active
            // state, so ESI must be different! Other frame flags MUST be the same,
            // otherwise we may get different frames!
            if (test_variant == TestVariant::CanFdEna) {
                frm_flags_2 = std::make_unique<FrameFlags>(
                    elem_test.frame_kind_, IdentKind::Base, RtrFlag::Data,
                    BrsFlag::NoShift, EsiFlag::ErrAct);

                gold_frm->frm_flags_ = *frm_flags_2;
            }
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);


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
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(0, BitKind::AckDelim)->val_ = BitVal::Dominant;

            Bit *eof_bit = drv_bit_frm->GetBitOf(0, BitKind::Eof);
            size_t eof_start = drv_bit_frm->GetBitIndex(eof_bit);

            drv_bit_frm->InsertPasErrFrm(eof_start);
            mon_bit_frm->InsertPasErrFrm(eof_start);

            size_t interm_index = drv_bit_frm->GetBitIndex(
                                    drv_bit_frm->GetBitOf(0, BitKind::Interm));
            drv_bit_frm->RemoveBitsFrom(interm_index);
            mon_bit_frm->RemoveBitsFrom(interm_index);

            if (elem_test.index_ == 1)
            {
                for (size_t i = 0; i < 1408; i++)
                {
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }

                // IUT specific compensation to exactly match after what time IUT
                // joins the bus. This checks exactly for given IUT, not the "minimum"
                // time as stated in the test! This is more strict, however it would
                // need to be adjusted for other implementation!
                // TODO: Genealize for other implementations than CTU CAN FD!
                for (size_t i = 0; i < 14; i++)
                {
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }
                Bit *last = mon_bit_frm->GetBit(mon_bit_frm->GetLen() - 1);
                last->GetTQ(0)->Lengthen(dut_input_delay);

            } else {

                for (size_t i = 0; i < 10; i++)
                {
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }
                drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Dominant);
                mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);

                for (size_t i = 0; i < 21; i++)
                {
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }
                drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Dominant);
                mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);

                for (size_t i = 0; i < 127; i++)
                {
                    for (size_t j = 0; j < 11; j++)
                    {
                        drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                        mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    }
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Dominant);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }
                drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);

                // IUT specific compensation to exactly match after what time IUT
                // joins the bus. This checks exactly for given IUT, not the "minimum"
                // time as stated in the test! This is more strict, however it would
                // need to be adjusted for other implementation!
                // TODO: Genealize for other implementations than CTU CAN FD!
                for (size_t i = 0; i < 11; i++)
                {
                    drv_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                    mon_bit_frm->AppendBit(BitKind::Idle, BitVal::Recessive);
                }
                Bit *last = mon_bit_frm->GetBit(mon_bit_frm->GetLen() - 1);
                last->GetTQ(0)->Lengthen(dut_input_delay);
            }

            // Re-transmitted frame after reintegration!
            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            dut_ifc->SetTec(255); /* just before bus-off */
            dut_ifc->SendReintegrationRequest(); /* Request in advance, DUT will hold it */
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            /* Must restart DUT for next iteration since it is bus off! */
            dut_ifc->Disable();
            dut_ifc->Reset();
            dut_ifc->ConfigureBitTiming(nbt, dbt);
            dut_ifc->Enable();

            TestMessage("Waiting till DUT is error active!");
            while (dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(2000);

            return FinishElemTest();
        }

};