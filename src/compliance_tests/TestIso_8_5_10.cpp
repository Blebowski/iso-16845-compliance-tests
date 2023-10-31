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
 * @date 2.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.5.10
 *
 * @brief The purpose of this test is to verify that a passive state IUT does
 *        not transmit a frame starting with an identifier and without
 *        transmitting SOF when detecting a dominant bit on the third bit of
 *        the intermission field.
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
 *      #1 The LT forces the bus to recessive for bus-off recovery time
 *         (22 bits).
 *
 * Setup:
 *  The IUT is set to the TEC passive state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame twice.
 *  The LT causes the IUT to generate an error frame. During the error flag
 *  transmitted by the IUT, the LT forces recessive state during 16 bit times.
 *  After the following passive error flag, the error delimiter is forced to
 *  dominant state for 112 bit times.
 *
 *  Then, the IUT transmits its first frame. The LT acknowledges the frame
 *  and immediately causes the IUT to generate an overload frame.
 *
 *  The LT forces the first bit of this overload flag to recessive state
 *  creating a bit error. (6 + 7) bit times later, the LT generates a dominant
 *  bit to cause the IUT to generate a new overload frame.
 *
 *  The LT forces the first bit of this new overload flag to recessive state
 *  causing the IUT to increments its TEC to the bus-off limit.
 *
 *  (6 + 8 + 3 + 8) bit times later, the LT sends a valid frame according to
 *  elementary test cases.
 *
 * Response:
 *  Only one frame shall be transmitted by the IUT.
 *  The IUT shall not acknowledge the frame sent by the LT.
 *  Error counter shall be reset after bus-off recovery.
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

class TestIso_8_5_10 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            n_elem_tests = 1;
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            AddElemTest(TestVariant::CanFdEna, ElemTest(1, FrameKind::CanFd));

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x80;

            /* First frame */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                            IdentKind::Base, RtrFlag::Data, BrsFlag::NoShift,
                            EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x8, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /* Second frame */
            frm_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                                                            RtrFlag::Data);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2);
            RandomizeAndPrint(gold_frm_2.get());

            /* At first, frm_2 holds the same retransmitted frame! */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force 7-th data bit to dominant to cause stuff error.
             *   2. Insert 16 dominant bits from next bit of monitored frame. Insert 16 recessive
             *      bits from next bit of driven frame. This emulates DUT always re-starting error
             *      detecting bit error during active error flag.
             *   3. Insert 6 recessive bits to emulate passive error flag (both driven and
             *      monitored frames).
             *   4. Insert 112 dominant bits to driven frame and 112 recessive bits to monitored
             *      frame. Then Insert 8 + 3 + 8 (Error delimiter + intermission + Suspend) to
             *      recessive frames. Compensate first monitored bit.
             *   5. Insert second frame as if transmitted by DUT. Append the same frame on driven
             *      frame since TX/RX feedback is disabled! This is the same frame as before
             *      because it is retransmitted by DUT!
             *   6. Force first bit of intermission to dominant state -> Overload condition. This
             *      is in fact 4th intermission bit (overall since there were) three bits before!
             *   7. Insert one dominant bit on monitored frame and one recessive bit on driven
             *      frame. This emulates expected first bit of overload flag and corruption of its
             *      first bit.
             *   8. Insert 6+7 recessive bits on both driven and monitored frames. This  emulates
             *      Passive error flag and error delimiter.
             *   9. Insert one dominant bit to driven frame, and one recessive bit to monitored
             *      frame. This represents next overload condition.
             *  10. Insert 1 recessive bit to driven frame (error on first bit of overload frame).
             *      Insert 1 dominant bit to monitored frame (first bit) of overload frame. This
             *      should cause DUT to go Bus-off.
             *  11. Insert 6 + 8 + 3 + 8 recessive bits to both driven and monitored frames. This
             *      corresponds to Passive Error flag, Error delimiter + Intermission + possible
             *      suspend.
             *  12. Insert third frame as if sent by LT. In driven frame, this frame is as if
             *      transmitted. In monitored frame, it is all recessive (including ACK) since IUT
             *      shall be bus-off.
             **************************************************************************************/
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            int index_to_remove = drv_bit_frm->GetBitIndex(
                                    drv_bit_frm->GetBitOf(7, BitKind::Data));
            drv_bit_frm->RemoveBitsFrom(index_to_remove);
            mon_bit_frm->RemoveBitsFrom(index_to_remove);

            for (int i = 0; i < 16; i++)
            {
                drv_bit_frm->AppendBit(BitKind::ActErrFlag, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::ActErrFlag, BitVal::Dominant);
            }

            for (int i = 0; i < 6; i++)
            {
                drv_bit_frm->AppendBit(BitKind::PasErrFlag, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::PasErrFlag, BitVal::Recessive);
            }

            for (int i = 0; i < 112; i++)
            {
                drv_bit_frm->AppendBit(BitKind::ActErrFlag, BitVal::Dominant);
                mon_bit_frm->AppendBit(BitKind::ActErrFlag, BitVal::Recessive);
            }

            // Compensate IUTs resynchronization caused by input delay due to first of 112
            // applied dominant bits. Recessive -> Dominant edge is applied right at SYNC,
            // due to input delay IUT will perceive this later and positively resynchronize.
            mon_bit_frm->GetBitOf(16, BitKind::ActErrFlag)
                ->GetFirstTQIter(BitPhase::Sync)->Lengthen(dut_input_delay);

            for (int i = 0; i < 8; i++)
            {
                drv_bit_frm->AppendBit(BitKind::ErrDelim, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::ErrDelim, BitVal::Recessive);
            }

            for (int i = 0; i < 3; i++)
            {
                drv_bit_frm->AppendBit(BitKind::Interm, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::Interm, BitVal::Recessive);
            }

            for (int i = 0; i < 8; i++)
            {
                drv_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::SuspTrans, BitVal::Recessive);
            }

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            /* Compensate ESI of second frame in second elementary test. Then IUT is already
             * passive!
             */
            if (test_variant == TestVariant::CanFdEna)
            {
                mon_bit_frm_2->GetBitOf(0, BitKind::Esi)->val_ = BitVal::Recessive;
                drv_bit_frm_2->GetBitOf(0, BitKind::Esi)->val_ = BitVal::Recessive;

                mon_bit_frm_2->UpdateFrame();
                drv_bit_frm_2->UpdateFrame();
            }

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            /* Actuall first bit of intermission after second frame */
            drv_bit_frm->GetBitOf(3, BitKind::Interm)->val_ = BitVal::Dominant;

            drv_bit_frm->GetBitOf(4, BitKind::Interm)->val_ = BitVal::Recessive;
            mon_bit_frm->GetBitOf(4, BitKind::Interm)->val_ = BitVal::Dominant;

            /* Remove last bit of intermission */
            drv_bit_frm->RemoveBit(drv_bit_frm->GetBitOf(5, BitKind::Interm));
            mon_bit_frm->RemoveBit(mon_bit_frm->GetBitOf(5, BitKind::Interm));

            for (int i = 0; i < 6; i++)
            {
                drv_bit_frm->AppendBit(BitKind::PasErrFlag, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::PasErrFlag, BitVal::Recessive);
            }
            for (int i = 0; i < 7; i++)
            {
                drv_bit_frm->AppendBit(BitKind::ErrDelim, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::ErrDelim, BitVal::Recessive);
            }

            /* Next overload condition */
            drv_bit_frm->AppendBit(BitKind::Interm, BitVal::Dominant);
            mon_bit_frm->AppendBit(BitKind::Interm, BitVal::Recessive);

            /* Error on first bit of overload flag */
            drv_bit_frm->AppendBit(BitKind::Interm, BitVal::Recessive);
            mon_bit_frm->AppendBit(BitKind::Interm, BitVal::Dominant);

            /* 6 + 8 + 8 + 3 frames */
            for (int i = 0; i < 25; i++)
            {
                /* Bit type in frame is don't care pretty much */
                drv_bit_frm->AppendBit(BitKind::Interm, BitVal::Recessive);
                mon_bit_frm->AppendBit(BitKind::Interm, BitVal::Recessive);
            }

            /* Append as if third frame which DUT shall not ACK (its bux off) */
            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            mon_bit_frm_2->ConvRXFrame();
            mon_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();

            CheckLTResult();

            /* Must restart DUT for next iteration since it is bus off! */
            this->dut_ifc->Disable();
            this->dut_ifc->Enable();

            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(2000);

            return FinishElemTest();
        }

};