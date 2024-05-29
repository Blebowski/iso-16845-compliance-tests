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
 * @date 09.7.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.3
 *
 * @brief This test verifies the capability of the IUT to manage the arbitration
 *        mechanism on every bit position in a base format frame it is
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
 *      For an OPEN device, there are, at most, 11 elementary tests to perform.
 *          Transmitted frame
 *    ID      RTR/RRS         DATA      Description of the     Number of elementary
 *                           field     concerned arbitration          tests
 *                                            bit
 *  0x7EF        0          DontShift Data     Collision on all bits          10
 *                                           equal to 1.
 *  0x010        0          DontShift Data     Collision on all bits          1
 *                                           equal to 1.
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

class TestIso_8_1_3 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 11; i++){
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
            /* We match bit position to be flipped with test index */
            if (elem_test.index_ == 7)
                id_iut = 0x010;
            else
                id_iut = 0x7EF;

            /* LT must have n-th bit of ID set to dominant */
            id_lt = id_iut;
            id_lt &= ~(1 << (11 - static_cast<int>(elem_test.index_)));

            /* In this test, we MUST NOT shift bit-rate! After loosing arbitration, IUT will
             * resynchronize in data bit-rate if granularity of data bit-rate is higher than
             * of nominal bit-rate! This would result in slightly shifted monitored frame
             * compared to IUT!
             */
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc, id_lt);
            RandomizeAndPrint(gold_frm.get());

            /* This frame is actually given to IUT to send it */
            gold_frm_2 = std::make_unique<Frame>(*frm_flags, dlc, id_iut);
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
             *      recessive.
             *   2. Loose arbitration on n-th bit of base identifier in monitored frame. Skip stuff
             *      bits!
             *   3. Compensate resynchronisation caused by IUTs input delay.
             *   4. Append second frame as if retransmitted by IUT. This one must be created from
             *      frame which was actually issued to IUT
             *************************************************************************************/
            Bit *loosing_bit =  mon_bit_frm->GetBitOfNoStuffBits(elem_test.index_ - 1,
                                                    BitKind::BaseIdent);
            loosing_bit->val_ = BitVal::Recessive;
            mon_bit_frm->LooseArbit(loosing_bit);

            loosing_bit->GetLastTQIter(BitPhase::Ph2)->Lengthen(dut_input_delay);

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