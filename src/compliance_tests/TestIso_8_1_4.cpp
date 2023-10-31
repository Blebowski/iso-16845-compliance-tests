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
 * @date 24.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.4
 *
 * @brief This test verifies the capacity of the IUT to manage the arbitration
 *        mechanism on every bit position in an extended format frame it is
 *        transmitting.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD Enabled
 *
 * Test variables:
 *      ID
 *      DLC
 *      FDF = 0
 *
 * Elementary test cases:
 *      For an OPEN device, there are, at most, 31 elementary tests to perform.
 *          Transmitted frame
 *    ID      RTR/RRS         DATA      Description of the     Number of elementary
 *                           field     concerned arbitration          tests
 *                                            bit
 *  0x1FBFFFFF   0           No Data    Collision on all bits           28
 *                            field         equal to 1.
 *  0x00400000   0           No Data    Collision on all bits           1
 *                            field         equal to 1.
 *  0x00400000   0           No Data    Collision on SRR and            2
 *                            field         IDE bit
 *
 *  For a SPECIFIC device, all possible possibilities of transmitting a recessive
 *  arbitration bit shall be considered.
 *
 *  For CAN FD enabled test, the RTR is represented by RRS and transmitted as 0.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT forces a recessive
 *  bit in the arbitration field to the dominant state according to the table in
 *  elementary test cases and continues to send a valid frame.
 *
 * Response:
 *  The IUT shall become receiver when sampling the dominant bit sent by the LT.
 *  As soon as the bus is idle, the IUT shall restart the transmission of the
 *  frame. The IUT shall not generate any error flag during the test.
 *  The content of the frame shall match the LT request.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_1_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (int i = 0; i < 31; i++) {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            /* Basic setup for tests where IUT transmits */
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc = 0x1;
            int id_iut;
            int id_lt;
            RtrFlag rtr_flag;

            /* We match bit position to be flipped with test index. Last two tests are tests where
             * we test on SRR or IDE
             */
            if (elem_test.index_ == 7 || elem_test.index_ == 30 || elem_test.index_ == 31)
                id_iut = 0x00400000;
            else
                id_iut = 0x1FBFFFFF;

            /* LT must have n-th bit of ID set to dominant */
            id_lt = id_iut;
            if (elem_test.index_ < 30)
                id_lt &= ~(1 << (29 - elem_test.index_));

            /* On elementary test 31, LT send base frame. Correct the ID so that LT sends base ID
             * with the same bits as IUT. This will guarantee that IUT sending extended frame with
             * 0x00400000 will send the same first bits as LT. Since LT will be sending base frame,
             * but IUT extended frame, IUT will loose on IDE bit!
            */
            else if (elem_test.index_ == 31)
                id_lt = (id_iut >> 18) & 0x7FF;

            /* On elementary test 31, IUT shall loose on IDE bit. Occurs when LT sends base frame! */
            IdentKind ident_type_lt;
            if (elem_test.index_ == 31)
                ident_type_lt = IdentKind::Base;
            else
                ident_type_lt = IdentKind::Ext;

            /* On elementary test 31, IUT shall loose on IDE bit. After Base ID IUT will send
             * recessive SRR. LT must also send recessive bit (RTR) otherwise IUT would loose on
             * SRR and not IDE!
             */
            if (elem_test.index_ == 31)
                rtr_flag = RtrFlag::Rtr;
            else
                rtr_flag = RtrFlag::Data;

            /* In this test, we MUST NOT shift bit-rate! After loosing arbitration, IUT will
             * resynchronize in data bit-rate if granularity of data bit-rate is higher than
             * of nominal bit-rate! This would result in slightly shifted monitored frame
             * compared to IUT!
             */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, ident_type_lt,
                                    rtr_flag, BrsFlag::NoShift, EsiFlag::ErrAct);
            frm_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_kind_,
                                    IdentKind::Ext, rtr_flag, BrsFlag::NoShift,
                                    EsiFlag::ErrAct);

            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, id_lt);
            RandomizeAndPrint(gold_frm.get());

            /* This frame is actually given to IUT to send it */
            gold_frm_2 = std::make_unique<Frame>(*frm_flags_2, dlc, id_iut);
            RandomizeAndPrint(gold_frm_2.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm_2);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force n-th bit of monitored frame to recessive. Monitored frame is created from
             *      golden_frame which has n-th bit dominant, but IUT is requested to send frame
             *      with this bit recessive (golden_frm_2). Therefore this bit shall be expected
             *      recessive. Bit position is calculated from elementary test index. First 11
             *      tests are in base identifier, next 18 are in Identifier extension. Indices 30
             *      and 31 correspond to bits SRR and IDE.
             *   2. Loose arbitration on n-th bit of base identifier in monitored frame. Skip stuff
             *      bits!
             *   3. Compensate IUTs input delay which will cause positive resynchronisation due
             *      to dominant bit being transmitted by IUT at bit in which IUT loses arbitration.
             *   4. Compensate length of CRC field in the monitored frame. Since the monitored frame
             *      gets a bit flipped, its original CRC is invalid. The actual CRC value does
             *      not matter because IUT lost arbitration. However, the original CRC may have
             *      different length due to possible stuff bits. IUT has lost arbitration, and
             *      thus it will received the driven frame. But, since in step 5 we append
             *      retransmitted frame, due to different CRC length we may see the retransmitted
             *      frame one bit shifted. This would cause monitor mismatch even if DUT behaves
             *      correctly. We need to make CRC of monitored frame equally long as in the
             *      driven frame.
             *   5. Append second frame as if retransmitted by IUT. This one must be created from
             *      frame which was actually issued to IUT
             *************************************************************************************/
            Bit *loosing_bit;

            if (elem_test.index_ < 12){
                loosing_bit = mon_bit_frm->GetBitOfNoStuffBits(elem_test.index_ - 1,
                                                        BitKind::BaseIdent);
            } else if (elem_test.index_ < 30){
                loosing_bit = mon_bit_frm->GetBitOfNoStuffBits(elem_test.index_ - 12,
                                                        BitKind::ExtIdent);
            /* Elementary test 30 - loose on SRR */
            } else if (elem_test.index_ == 30) {
                loosing_bit = mon_bit_frm->GetBitOf(0, BitKind::Srr);

            /* Elementary test 31 - loose on IDE */
            } else if (elem_test.index_ == 31){
                loosing_bit = mon_bit_frm->GetBitOf(0, BitKind::Ide);

            } else {
                loosing_bit = mon_bit_frm->GetBitOf(0, BitKind::Ide);
                TestMessage("Invalid Elementary test index: %d", elem_test.index_);
            }

            loosing_bit->val_ = BitVal::Recessive;
            mon_bit_frm->LooseArbit(loosing_bit);

            loosing_bit->GetLastTQIter(BitPhase::Ph2)->Lengthen(dut_input_delay);

            /* On elementary test 30, IUT shall loose on SRR bit, therefore we must send this bit
             * dominant by LT, so we flip it
             */
            if (elem_test.index_ == 30){
                Bit *srr_bit = drv_bit_frm->GetBitOf(0, BitKind::Srr);
                srr_bit->val_ = BitVal::Dominant;
                int index = drv_bit_frm->GetBitIndex(srr_bit);

                /* Forcing SRR low will cause 5 consecutive dominant bits at the end of base ID,
                 * therefore IUT inserts recessive stuff bit. Model does not account for this,
                 * so we must insert one extra bit in monitored frame. For driven frame, we must
                 * recalculate CRC!
                 */
                drv_bit_frm->UpdateFrame();
                mon_bit_frm->InsertBit(BitKind::Srr, BitVal::Recessive, index + 1);
            }

            /* On elementary test 31, IUT will be sending extended frame with the same base ID as
             * LT. LT will be sending Base frame. But monitored frame is constructed from LTs
             * frame which always has RTR bit dominant (right after Base ID). IUT is sending
             * Extended frame, therefore at position of RTR it will send SRR which is Recessive.
             * So bit at position of RTR in monitored frame must be set to recessive!
             *
             * Note that in CAN FD frame there is no RTR bit so R1 must be set instead!
             */
            if (elem_test.index_ == 31) {
                if (test_variant == TestVariant::Common)
                    mon_bit_frm->GetBitOf(0, BitKind::Rtr)->val_ = BitVal::Recessive;
                else
                    mon_bit_frm->GetBitOf(0, BitKind::R1)->val_ = BitVal::Recessive;
            }

            // Compensate CRC length in monitored frame
            while (mon_bit_frm->GetFieldLen(BitKind::Crc) <
                   drv_bit_frm->GetFieldLen(BitKind::Crc)) {
                int index = mon_bit_frm->GetBitIndex(
                                mon_bit_frm->GetBitOf(0, BitKind::Crc));
                mon_bit_frm->InsertBit(BitKind::Crc, BitVal::Recessive, index);
            }

            while (mon_bit_frm->GetFieldLen(BitKind::Crc) >
                   drv_bit_frm->GetFieldLen(BitKind::Crc)) {
                mon_bit_frm->RemoveBit(mon_bit_frm->GetBitOf(0, BitKind::Crc));
            }

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
            this->dut_ifc->SendFrame(gold_frm_2.get());
            WaitForDrvAndMon();
            CheckLTResult();
            CheckRxFrame(*gold_frm);

            FreeTestObjects();
            return FinishElemTest();
        }

};