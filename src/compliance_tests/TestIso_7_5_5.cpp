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
 * @date 30.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.5.5
 *
 * @brief The purpose of this test is to verify that an error passive IUT
 *        restarts the passive error flag when detecting up to 5 consecutive
 *        dominant bits during its own passive error flag.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Passive error flag, FDF = 0
 *
 *  CAN FD Enabled
 *      Passive error flag, FDF = 1
 *
 * Elementary test cases:
 *  Elementary tests to perform:
 *      Elementary tests to perform superimposing the passive error flag by
 *      the sequence of 5 dominant bits starting at
 *          #1 the first bit of the passive error flag,
 *          #2 the third bit of the passive error flag, and
 *          #3 the sixth bit of the passive error flag.
 *
 * Setup:
 *  The IUT is set in passive state.
 *
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  During the passive error flag sent by the IUT, the LT sends a sequence
 *  of 5 dominant bits according to elementary test cases.
 *  After this sequence, the LT waits for (6 + 7) bit time before sending a
 *  dominant bit, corrupting the last bit of the error delimiter.
 *
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the last dominant bit sent by the LT.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_5_5 : public test::TestBase
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

            dut_ifc->SetTec((rand() % 110) + 128);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &error_data);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff bit
             *      therefore causing error.
             *   3. Insert Passive Error frame to both driven and monitored frames from next bit.
             *   4. Insert 5 dominant bits to driven frame from 1/3/6-th bit of passive error flag.
             *   5. Insert passive error flag from one bit beyond the last dominat bit from previous
             *      step. Insert to both driven and monitored frames.
             *   6. Flip last bit of new error delimiter to dominant (overload flag).
             *   7. Insert overload flag expected from next bit on to both driven and monitored
             *      frames.
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertPasErrFrm(7, BitKind::Data);

            size_t where_to_insert;
            if (elem_test.index_ == 1)
                where_to_insert = 0;
            else if (elem_test.index_ == 2)
                where_to_insert = 2;
            else
                where_to_insert = 5;
            size_t bit_index = drv_bit_frm->GetBitIndex(
                                drv_bit_frm->GetBitOf(where_to_insert, BitKind::PasErrFlag));

            for (size_t i = 0; i < 5; i++)
            {
                drv_bit_frm->InsertBit(BitKind::ActErrFlag, BitVal::Dominant, bit_index);
                mon_bit_frm->InsertBit(BitKind::PasErrFlag, BitVal::Recessive, bit_index);
            }

            /* Next Passive Error flag should start right after 5 inserted bits */
            drv_bit_frm->InsertPasErrFrm(bit_index + 5);
            mon_bit_frm->InsertPasErrFrm(bit_index + 5);

            /*
             * Now the only bits of error delimiter should be the ones from last error
             * delimiter because it overwrote previous ones!
             */
            drv_bit_frm->GetBitOf(7, BitKind::ErrDelim)->FlipVal();

            drv_bit_frm->InsertOvrlFrm(0, BitKind::Interm);
            mon_bit_frm->InsertOvrlFrm(0, BitKind::Interm);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);
            CheckLTResult();
            CheckNoRxFrame();

            FreeTestObjects();
            return FinishElemTest();
        }
};