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
 * @date 15.8.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.4.4
 *
 * @brief This test verifies that an IUT acting as a transmitter detects a bit
 *        error when one of the 6 dominant bits of the overload flag it
 *        transmits is forced to recessive state by LT.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *          FDF = 0
 *      CAN FD Enabled:
 *          FDF = 1
 *
 * Elementary test cases:
 *      For OPEN devices, the identifier shall start with 4 dominant bits.
 *      For a SPECIFIC device which cannot send such an identifier, any other
 *      value may be used.
 *
 *      Elementary tests to perform:
 *          #1 corrupting the first bit of the overload flag;
 *          #2 corrupting the second bit of the overload flag;
 *          #3 corrupting the sixth bit of the overload flag.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  Then, the LT causes the IUT to generate an overload frame.
 *  Then, the LT corrupts one of the 6 dominant bits of the overload flag to
 *  the recessive state according to elementary test cases.
 *
 * Response:
 *  The IUT shall generate an error frame starting at the bit position after
 *  the corrupted bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_4_4 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            for (size_t i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElemTest(i + 1, FrameKind::Can20));
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));
            }

            /* Standard settings for tests where IUT is transmitter */
            SetupMonitorTxTests();
            /* Dont enable TX to RX feedback beacuse we need to force Dominant overload flag to
             * be received as Recessive!
             */
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Force ACK low in driven frame (TX/RX feedback not enabled!)
             *  2. Force first bit of Intermission to Dominant (Overload condition).
             *  3. Insert Overload frame from second bit of Intermission to monitored frame.
             *  4. Force 1, 2, 6-th bit of Overload flag to Recessive.
             *  5. Insert Active Error frame from next bit to driven frame. Insert Active Error
             *     frame to monitored frame.
             *
             *  Note: Don't insert retransmitted frame after first frame, since error happened in
             *        overload frame which was transmitted due to Overload condition in
             *        Intermission. At this point frame has already been validated by transmitter!
             *        This is valid according to spec. since for transmitter frame vaidation shall
             *        occur at the end of EOF!
             *************************************************************************************/
            drv_bit_frm->PutAck(dut_input_delay);

            Bit *first_interm_bit = drv_bit_frm->GetBitOf(0, BitKind::Interm);
            drv_bit_frm->FlipBitAndCompensate(first_interm_bit, dut_input_delay);

            drv_bit_frm->InsertOvrlFrm(1, BitKind::Interm);
            mon_bit_frm->InsertOvrlFrm(1, BitKind::Interm);

            Bit *bit_to_corrupt;
            if (elem_test.index_ == 1){
                bit_to_corrupt = drv_bit_frm->GetBitOf(0, BitKind::OvrlFlag);
            } else if (elem_test.index_ == 2){
                bit_to_corrupt = drv_bit_frm->GetBitOf(1, BitKind::OvrlFlag);
            } else {
                bit_to_corrupt = drv_bit_frm->GetBitOf(5, BitKind::OvrlFlag);
            }

            size_t bit_index = drv_bit_frm->GetBitIndex(bit_to_corrupt);
            bit_to_corrupt->val_ = BitVal::Recessive;

            drv_bit_frm->InsertActErrFrm(bit_index + 1);
            mon_bit_frm->InsertActErrFrm(bit_index + 1);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            this->dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};