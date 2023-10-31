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
 * @date 20.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.22
 *
 * @brief This test verifies that the IUT increases its REC by 1 when
 *        detecting a form error.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  CAN FD Enabled
 *      REC
 *      DLC - to cause different CRC types
 *      FDF = 1
 *
 * Elementary test cases:
 *   Elementary tests to perform on recessive stuff bits:
 *      #1 DLC ≤ 10 − > CRC (17) field;
 *      #2 DLC > 10 − > CRC (21) field.
 *   Elementary tests to perform on dominant stuff bits:
 *      #3 DLC ≤ 10 − > CRC (17) field
 *      #4 DLC > 10 − > CRC (21) field
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT corrupts a fixed stuff bit according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 on the corrupted fixed stuff
 *  bit.
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_22 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CanFdEnaOnly);
            for (int i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t dlc;
            BitVal bit_value;

            /* Tests 1,3 -> DLC < 10. Tests 2,4 -> DLC > 10 */
            if (elem_test.index_ % 2 == 0)
                dlc = (rand() % 5) + 0xA;
            else
                dlc = rand() % 10;

            if (elem_test.index_ < 3)
                bit_value = BitVal::Recessive;
            else
                bit_value = BitVal::Dominant;

            frm_flags = std::make_unique<FrameFlags>(FrameKind::CanFd, RtrFlag::Data);
            gold_frm = std::make_unique<Frame>(*frm_flags, dlc);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitored frame to received frame.
             *   2. Pick one of the stuff bits with required value (can be only in CRC field or
             *      stuff count!) and flip its value.
             *   3. Insert Active Error frame to monitored frame. Insert Passive Error frame to
             *      driven frame (TX/RX feedback enabled).
             *************************************************************************************/
            mon_bit_frm->ConvRXFrame();

            int num_stuff_bits = drv_bit_frm->GetNumStuffBits(StuffKind::Fixed,
                                                                 bit_value);
            printf("Number of fixed stuff bits matching: %d\n", num_stuff_bits);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            for (int stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
            {
                TestMessage("Testing stuff bit nr: %d", stuff_bit);
                stuff_bits_in_variant++;

                /*
                 * Copy frame to second frame so that we dont loose modification of bits.
                 * Corrupt only second one.
                 */
                drv_bit_frm_2 = std::make_unique<BitFrame>(*drv_bit_frm);
                mon_bit_frm_2 = std::make_unique<BitFrame>(*mon_bit_frm);

                /* Get stuff bit on 'stuff_bit' position */
                Bit *stuff_bit_to_flip = drv_bit_frm_2->GetFixedStuffBit(stuff_bit, bit_value);

                int bit_index = drv_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                stuff_bit_to_flip->FlipVal();

                drv_bit_frm_2->InsertPasErrFrm(bit_index + 1);
                mon_bit_frm_2->InsertActErrFrm(bit_index + 1);

                drv_bit_frm_2->Print(true);
                mon_bit_frm_2->Print(true);

                /* Test itself */
                rec_old = dut_ifc->GetRec();
                PushFramesToLT(*drv_bit_frm_2, *mon_bit_frm_2);
                RunLT(true, true);

                CheckLTResult();
                CheckRecChange(rec_old, +1);
                if (test_result == false)
                    return false;
            }

            FreeTestObjects();
            return FinishElemTest();
        }
};