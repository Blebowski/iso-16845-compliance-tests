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
 * @test ISO16845 7.6.9
 *
 * @brief This test verifies that the IUT increases its REC by 1 when
 *        detecting a stuff error.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 *
 *  CAN FD Enabled
 *      REC, FDF = 1
 *
 * Elementary test cases:
 *  Classical CAN, CAN FD tolerant, CAN FD enabled:
 *      Elementary tests to perform on recessive stuff bits"
 *          #1 arbitration field;
 *          #2 control field;
 *          #3 data field;
 *          #4 CRC field.
 *      Elementary tests to perform on dominant stuff bits:
 *          #5 arbitration field;
 *          #6 control field;
 *          #7 data field;
 *          #8 CRC field.
 *
 *  CAN FD enabled:
 *      Elementary tests to perform on recessive stuff bits:
 *          #1 arbitration field;
 *          #2 control field;
 *          #3 data field.
 *
 *      Elementary tests to perform on dominant stuff bits:
 *          #4 arbitration field;
 *          #5 control field;
 *          #6 data field.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a sequence of 6 consecutive bits according to elementary
 *  test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 on the sixth consecutive bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <bitset>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_7_6_9 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 8; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
            for (int i = 0; i < 6; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        BitType get_rand_arbitration_field()
        {
            switch (rand() % 5)
            {
            case 0:
                return BitType::BaseIdentifier;
            case 1:
                return BitType::IdentifierExtension;
            case 2:
                return BitType::Rtr;
            case 3:
                return BitType::Ide;
            case 4:
                return BitType::Srr;
            default:
                break;
            }
            return BitType::BaseIdentifier;
        }

        BitType get_rand_control_field()
        {
            switch (rand() % 5)
            {
            case 0:
                return BitType::R0;
            case 1:
                return BitType::R1;
            case 2:
                return BitType::Brs;
            case 3:
                return BitType::Esi;
            case 4:
                return BitType::Dlc;
            default:
                break;
            }
            return BitType::Dlc;
        }

        /**
         * Generates frame, makes sure that required bit field contains stuff bit
         * of required value.
         * @returns Index of stuff bit (within whole frame) representing stuff bit
         *          of given value within a bit field.
         */
        int GenerateFrame(TestVariant test_variant, ElementaryTest elem_test)
        {
            BitType field = BitType::Sof;
            BitValue value = BitValue::Dominant;

            if (test_variant == TestVariant::Common)
            {
                assert(elem_test.index_ > 0 && elem_test.index_ < 9);
                switch (elem_test.index_)
                {
                case 1:
                case 5:
                    field = BitType::BaseIdentifier;
                    //field = get_rand_arbitration_field();
                    break;
                case 2:
                case 6:
                    field = BitType::Dlc;
                    //field = get_rand_control_field();
                    break;
                case 3:
                case 7:
                    field = BitType::Data;
                    break;
                case 4:
                case 8:
                    field = BitType::Crc;
                    break;
                default:
                    break;
                }

                if (elem_test.index_ < 5)
                    value = BitValue::Recessive;
                else
                    value = BitValue::Dominant;

            } else if (test_variant == TestVariant::CanFdEnabled) {
                assert(elem_test.index_ > 0 && elem_test.index_ < 7);
                switch (elem_test.index_)
                {
                case 1:
                case 4:
                    field = BitType::BaseIdentifier;
                    //field = get_rand_arbitration_field();
                    break;
                case 2:
                case 5:
                    field = BitType::Dlc;
                    //field = get_rand_control_field();
                    break;
                case 3:
                case 6:
                    field = BitType::Data;
                default:
                    break;
                }

                if (elem_test.index_ < 4)
                    value = BitValue::Recessive;
                else
                    value = BitValue::Dominant;
            }

            /* Corrupting dominant stuff bit in control field is special! We need to make sure that
             * there will be such stuff bit!
             * For frame with FDF = 0:
             *  All DLC bits recessive, force r0 recessive
             * For frame with FDF = 1:
             *  All DLC bits recessive, BRS = 1, ESI = 1
             */
            if (test_variant == TestVariant::Common && elem_test.index_ == 6) {
                frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_,
                                                            IdentifierType::Extended);
            } else if (test_variant == TestVariant::CanFdEnabled && elem_test.index_ == 5) {
                frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, BrsFlag::Shift,
                                                            EsiFlag::ErrorPassive);
            } else {
                frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, RtrFlag::DataFrame);
            }

            /* Search for stuff bit of desired value in field as given by elementary test.
             * If not succefull, then generate the frame again.
             */
            int num_stuff_bits = 0;
            TestMessage("Searching for: %d\n", field);
            TestMessage("Value: %d\n", value);

            while (num_stuff_bits == 0){
                std::cout << "Generating frame...\n";

                /* Again, special treament of dominant stuff bit in control field */
                if ((test_variant == TestVariant::Common && elem_test.index_ == 6) ||
                    (test_variant == TestVariant::CanFdEnabled && elem_test.index_ == 5)) {
                    golden_frm = std::make_unique<Frame>(*frame_flags, 0xF);
                } else {
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                }
                golden_frm->Randomize();

                // std::cout << "Identifier: " <<
                // std::bitset<29>(golden_frm->identifier()).to_string() << std::endl;

                driver_bit_frm = ConvertBitFrame(*golden_frm);

                /* To have recessive DLC stuff bit possible by randomization,
                 * R0 bit must be forced to dominant for CAN 2.0 frame */
                if (test_variant == TestVariant::Common && elem_test.index_ == 6)
                {
                    driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                    driver_bit_frm->UpdateFrame();
                }

                num_stuff_bits = driver_bit_frm->GetNumStuffBits(field,
                                    StuffBitType::NormalStuffBit, value);
                TestMessage("Number of matching stuff bits: %d\n", num_stuff_bits);
            }
            TestBigMessage("Found frame with required stuff-bits!");

            monitor_bit_frm = ConvertBitFrame(*golden_frm);
            return driver_bit_frm->GetBitIndex(
                        driver_bit_frm->GetStuffBit(field, StuffBitType::NormalStuffBit, value));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* Generate frame takes care of frame creation */
            int bit_to_corrupt = GenerateFrame(test_variant, elem_test);

            /* For now skip elementary test number 6 of CAN 2.0 variant!
             * This is because if we want to achive dominant stuff bit in control field
             * of CAN 2.0 frame, we have to force R0 bit to recessive to have enough
             * of equal consecutive recessive bits! If we do it, the IUT interprets this
             * r0 bit which comes right after IDE as EDL and goes to r0 of FD frame. There
             * it detects recessive and files erro so IUT must have protocol exception
             * configured! Alternative is to manufacture TC which will be RTR with Extended
             * ID ending with 4 recessive bits! Then first bit of Control field will be
             * a dominant stuff bit! This is TODO!
             */
            //if ((test_variant == TestVariant::Common && elem_test.index == 6)
            //    ||
            //    (test_variant == TestVariant::CanFdEnabled && elem_test.index == 5))
            //    return 0;

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force Stuff bit within it field as given by elementary test to opposite value.
             *   3. Insert Active Error flag from next bit on to monitored frame. Insert passive
             *      frame to driven frame (TX/RX feedback enabled).
             **************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBit(bit_to_corrupt)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(bit_to_corrupt + 1);
            monitor_bit_frm->InsertActiveErrorFrame(bit_to_corrupt + 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);

            CheckLowerTesterResult();
            CheckNoRxFrame();
            CheckRecChange(rec_old, +1);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};