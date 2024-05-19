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
 * @date 2.10.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.6.10
 *
 * @brief This test verifies that the IUT increases its REC by 1 when detecting
 *        a CRC error during reception of a frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, ACK = 1 Bit recessive, FDF = 0
 *
 *  CAN FD Enabled
 *      REC, DLC to cause different CRC types, ACK = 2 Bit recessive
 *      FDF = 1
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD tolerant, CAN FD enabled:
 *      There is one elementary test to perform:
 *          #1 CRC (15) error
 *
 *  CAN FD enabled:
 *      Elementary tests to perform:
 *          #1 DLC ≤ 10 − > CRC (17) error
 *          #2 DLC > 10 − > CRC (21) error
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing an error according to elementary test
 *  cases.
 *
 * Response:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled:
 *      The IUT sends a recessive acknowledge.
 *      The IUT starts the transmission of an active error frame at the first
 *      bit position following the ACK delimiter.
 *      The IUT’s REC value shall be increased by 1 by starting the error
 *      frame.
 *  CAN FD enabled:
 *      The IUT sends a recessive acknowledge.
 *      The IUT starts the transmission of an active error frame at the fourth
 *      bit position following the CRC delimiter.
 *      The IUT’s REC value shall be increased by 1 by starting the error
 *      frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <bitset>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_10 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElemTest(1, FrameKind::Can20));
            for (size_t i = 0; i < 2; i++)
                AddElemTest(TestVariant::CanFdEna, ElemTest(i + 1, FrameKind::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, RtrFlag::Data);
            if (test_variant == TestVariant::Common)
            {
                gold_frm = std::make_unique<Frame>(*frm_flags);
            } else {
                uint8_t dlc;
                if (elem_test.index_ == 1)
                    dlc = static_cast<uint8_t>(rand() % 0xB);
                else
                    dlc = static_cast<uint8_t>((rand() % 0x4) + 0xB);
                gold_frm = std::make_unique<Frame>(*frm_flags, dlc);
            }
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Choose random bit of CRC which is not stuff bit and flip its value. This has
             *      drawback, that it can change lenght of CRC at IUTs input since it can alter
             *      number of equal consecutive bits causing stuff bit insertion/drop!
             *      We must therefore update frame without CRC recalculation (to keep CRC error)!
             *   2. Monitor frame as if received. Force ACK low in monitored frame since IUT shall
             *      not send ACK then!
             *   3. Insert Active Error flag from first bit of EOF.
             *************************************************************************************/
            size_t crc_bit_index;
            size_t crc_overall_index;
            Bit *crc_bit;

            do {
                crc_bit_index = static_cast<size_t>(rand()) % drv_bit_frm->GetFieldLen(BitKind::Crc);
                crc_bit = drv_bit_frm->GetBitOf(crc_bit_index, BitKind::Crc);
                crc_overall_index = drv_bit_frm->GetBitIndex(crc_bit);
            } while (crc_bit->stuff_kind_ != StuffKind::NoStuff);

            crc_bit->FlipVal();
            mon_bit_frm->GetBit(crc_overall_index)->FlipVal();

            drv_bit_frm->UpdateFrame(false);
            mon_bit_frm->UpdateFrame(false);

            mon_bit_frm->ConvRXFrame();
            mon_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Recessive;

            drv_bit_frm->InsertPasErrFrm(0, BitKind::Eof);
            mon_bit_frm->InsertActErrFrm(0, BitKind::Eof);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            rec_old = dut_ifc->GetRec();
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            RunLT(true, true);

            CheckLTResult();
            CheckNoRxFrame();
            CheckRecChange(rec_old, +1);

            FreeTestObjects();
            return FinishElemTest();
        }
};