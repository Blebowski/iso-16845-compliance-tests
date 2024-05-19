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
 * @date 10.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.1.3
 *
 * @brief This test verifies the capability of the IUT to manage the reception
 *        of arbitration winning frame, while the IUT loses the arbitration.
 *
 * @version CAN FD Enabled, CAN FD Tolerant, Classical CAN
 *
 * Test variables:
 *  ID all bit = 1
 *  IDE
 *  SRR (in case of IDE = 1)
 *  FDF
 *  DLC = 0
 *  RTR = 1
 *
 * Elementary test cases:
 *  CAN FD Enabled, CAN FD Tolerant, Classical CAN :
 *          LT Frame format         IUT frame format        Bit arb. lost
 *      #1      CBFF                    CBFF                    RTR
 *      #2      CBFF                    CEFF                    SRR
 *      #3      CBFF                    CEFF                    IDE
 *      #4      CEFF                    CBFF                LSB Base ID
 *      #5      CEFF                    CEFF                LSB Extended ID
 *      #6      CEFF                    CEFF                    RTR
 *
 *  CAN FD Enabled :
 *          LT Frame format         IUT frame format        Bit arb. lost
 *      #1      CBFF                    FBFF                LSB Base ID
 *      #2      FBFF                    CBFF                    RTR
 *      #3      CEFF                    FEFF                LSB Extended ID
 *      #4      FEFF                    CEFF                    RTR
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to “IUT frame format”
 *  in elementary test cases. Then, the LT forces the bit described at “bit
 *  for arbitration lost” in elementary test cases to dominant state and
 *  continues to send a valid frame according to elementary test cases.
 *
 * Response:
 *  The IUT shall become the receiver when sampling the dominant bit sent by
 *  the LT.
 *  The frame received by the IUT shall match the frame sent by the LT.
 *  As soon as the bus is idle again, the IUT shall restart the transmission
 *  of the frame.
 *  The IUT shall not generate any error flag during the test.
 *  The content of the frame shall match the LT request.
 *
 * Note:
 *  An implementation with limited ID range may not be able to transmit/receive
 *  the frame.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_1_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 6; i++)
                AddElemTest(TestVariant::Common, ElemTest(i + 1));
            for (size_t i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(10));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);

            // Following constraint is not due to model or IUT issues.
            // It is due to principle of the test, we can't avoid it!
            // This is
            assert(dbt.brp_ == nbt.brp_ &&
                   " In this test BRP(N) must be equal to BRP(D) due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            IdentKind lt_id_type;
            IdentKind iut_id_type;

            RtrFlag lt_rtr_flag = RtrFlag::Data;
            RtrFlag iut_rtr_flag = RtrFlag::Data;

            int lt_id;
            int iut_id;

            FrameKind lt_frame_type;
            FrameKind iut_frame_type;

            if (test_variant == TestVariant::Common)
            {
                lt_frame_type = FrameKind::Can20;
                iut_frame_type = FrameKind::Can20;

                switch (elem_test.index_)
                {
                case 1:
                    lt_id_type = IdentKind::Base;
                    iut_id_type = IdentKind::Base;
                    iut_rtr_flag = RtrFlag::Rtr;
                    lt_id = rand() % CAN_BASE_ID_MAX;
                    iut_id = lt_id;
                    break;
                case 2:
                    lt_id_type = IdentKind::Base;
                    iut_id_type = IdentKind::Ext;
                    lt_id = rand() % CAN_BASE_ID_MAX;
                    iut_id = (lt_id << 18);
                    break;
                case 3:
                    lt_id_type = IdentKind::Base;
                    iut_id_type = IdentKind::Ext;
                    lt_rtr_flag = RtrFlag::Rtr;
                    lt_id = rand() % CAN_BASE_ID_MAX;
                    iut_id = (lt_id << 18);
                    break;
                case 4:
                    lt_id_type = IdentKind::Ext;
                    iut_id_type = IdentKind::Base;
                    iut_id = 0x7FF;
                    lt_id = (0x7FE << 18);
                    break;
                case 5:
                    lt_id_type = IdentKind::Ext;
                    iut_id_type = IdentKind::Ext;
                    lt_id = 0x1FFFFFFE;
                    iut_id = 0x1FFFFFFF;
                    break;
                case 6:
                    lt_id_type = IdentKind::Ext;
                    iut_id_type = IdentKind::Ext;
                    iut_rtr_flag = RtrFlag::Rtr;
                    lt_id = rand() % CAN_EXTENDED_ID_MAX;
                    iut_id = lt_id;
                    break;
                default:
                    break;
                }
            } else if (test_variant == TestVariant::CanFdEna) {
                switch (elem_test.index_)
                {
                case 1:
                    lt_frame_type = FrameKind::Can20;
                    iut_frame_type = FrameKind::CanFd;
                    lt_id_type = IdentKind::Base;
                    iut_id_type = IdentKind::Base;
                    lt_id = 0x3FE;
                    iut_id = 0x3FF;
                    break;
                case 2:
                    lt_frame_type = FrameKind::CanFd;
                    iut_frame_type = FrameKind::Can20;
                    lt_id_type = IdentKind::Base;
                    iut_id_type = IdentKind::Base;
                    lt_id = rand() % CAN_BASE_ID_MAX;
                    iut_id = lt_id;
                    iut_rtr_flag = RtrFlag::Rtr;
                    break;
                case 3:
                    lt_frame_type = FrameKind::Can20;
                    iut_frame_type = FrameKind::CanFd;
                    lt_id_type = IdentKind::Ext;
                    iut_id_type = IdentKind::Ext;
                    lt_id = 0x1FFFFFFE;
                    iut_id = 0x1FFFFFFF;
                    break;
                case 4:
                    lt_frame_type = FrameKind::CanFd;
                    iut_frame_type = FrameKind::Can20;
                    lt_id_type = IdentKind::Ext;
                    iut_id_type = IdentKind::Ext;
                    iut_rtr_flag = RtrFlag::Rtr;
                    lt_id = rand() % CAN_EXTENDED_ID_MAX;
                    iut_id = lt_id;
                    break;
                default:
                    break;
                }
            }

            /* For IUT */
            frm_flags = std::make_unique<FrameFlags>(iut_frame_type, iut_id_type, iut_rtr_flag,
                                                        EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x0, iut_id);
            RandomizeAndPrint(gold_frm.get());

            /* For LT */
            frm_flags_2 = std::make_unique<FrameFlags>(lt_frame_type, lt_id_type, lt_rtr_flag,
                                                        EsiFlag::ErrAct);
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2, 0x0, lt_id);
            RandomizeAndPrint(gold_frm_2.get());

            /* Driven/monitored is derived from LTs frame since this one wins over IUTs frame! */
            drv_bit_frm = ConvBitFrame(*gold_frm_2);
            mon_bit_frm = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Loose arbitration in monitored frame on bit according to elementary test cases.
             *      Correct monitored bit value to expect value which corresponds to what was sent
             *      by IUT!
             *   2. Compensate input delay of IUT. If Recessive to Dominant edge of bit on which
             *      arbitration is lost, is sent by LT exactly at SYNC, then due to input delay,
             *      it will be seen slightly later by IUT. If prescaler is smaller than input
             *      delay, this will create undesirable resynchronisation by IUT. This needs to
             *      be compensated by lenghtening monitored bit!
             *   3. Append retransmitted frame. This second frame is the frame sent by IUT. On
             *      driven frame as if received, on monitored as if transmitted by IUT. Use the
             *      frame which is issued to IUT for sending!
             *************************************************************************************/

            /* Initialize to make linter happy! */
            Bit *bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::Sof);
            if (test_variant == TestVariant::Common)
            {
                switch (elem_test.index_)
                {
                case 1:
                    bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::Rtr);
                    break;
                case 2:
                    /* In IUTs frame SRR is on RTR position of driven frame */
                    bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::Rtr);
                    break;
                case 3:
                    bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::Ide);
                    break;
                case 4:
                    bit_to_loose_arb = mon_bit_frm->GetBitOfNoStuffBits(
                                            10, BitKind::BaseIdent);
                    break;
                case 5:
                    bit_to_loose_arb = mon_bit_frm->GetBitOfNoStuffBits(17,
                                        BitKind::ExtIdent);
                    break;
                case 6:
                    bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::Rtr);
                default:
                    break;
                }
            } else if (test_variant == TestVariant::CanFdEna) {
                switch (elem_test.index_)
                {
                case 1:
                    bit_to_loose_arb = mon_bit_frm->GetBitOfNoStuffBits(10,
                                        BitKind::BaseIdent);
                    break;
                case 2:
                    /* In IUTs frame R1 is on position of RTR */
                    bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::R1);
                    break;
                case 3:
                    bit_to_loose_arb = mon_bit_frm->GetBitOfNoStuffBits(17,
                                        BitKind::ExtIdent);
                    break;
                case 4:
                    /* In IUTs frame R1 is on position of RTR */
                    bit_to_loose_arb = mon_bit_frm->GetBitOf(0, BitKind::R1);
                default:
                    break;
                }
            }

            /*
             * In all frames monitored bits shall be equal to driven bits up to point where
             * arbitration is lost!
             */
            bit_to_loose_arb->val_ = BitVal::Recessive;
            mon_bit_frm->LooseArbit(bit_to_loose_arb);

            /* Compensate input delay, lenghten bit on which arbitration was lost */
            bit_to_loose_arb->GetLastTQIter(BitPhase::Ph2)->Lengthen(dut_input_delay);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);
            drv_bit_frm_2->ConvRXFrame();

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            /* IUTs frame is sent! */
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();
            /* Received should be the frame which is sent by LT */
            CheckRxFrame(*gold_frm_2);

            FreeTestObjects();
            return FinishElemTest();
        }

};