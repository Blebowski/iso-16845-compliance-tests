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
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 6; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1));
            for (int i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(10));
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            IdentifierType lt_id_type;
            IdentifierType iut_id_type;

            RtrFlag lt_rtr_flag = RtrFlag::DataFrame;
            RtrFlag iut_rtr_flag = RtrFlag::DataFrame;

            int lt_id;
            int iut_id;

            FrameType lt_frame_type;
            FrameType iut_frame_type;

            if (test_variant == TestVariant::Common)
            {
                lt_frame_type = FrameType::Can2_0;
                iut_frame_type = FrameType::Can2_0;

                switch (elem_test.index_)
                {
                case 1:
                    lt_id_type = IdentifierType::Base;
                    iut_id_type = IdentifierType::Base;
                    iut_rtr_flag = RtrFlag::RtrFrame;
                    lt_id = rand() % (int)pow(2, 11);
                    iut_id = lt_id;
                    break;
                case 2:
                    lt_id_type = IdentifierType::Base;
                    iut_id_type = IdentifierType::Extended;
                    lt_id = rand() % (int)pow(2, 11);
                    iut_id = (lt_id << 18);
                    break;
                case 3:
                    lt_id_type = IdentifierType::Base;
                    iut_id_type = IdentifierType::Extended;
                    lt_rtr_flag = RtrFlag::RtrFrame;
                    lt_id = rand() % (int)pow(2, 11);
                    iut_id = (lt_id << 18);
                    break;
                case 4:
                    lt_id_type = IdentifierType::Extended;
                    iut_id_type = IdentifierType::Base;
                    iut_id = 0x7FF;
                    lt_id = (0x7FE << 18);
                    break;
                case 5:
                    lt_id_type = IdentifierType::Extended;
                    iut_id_type = IdentifierType::Extended;
                    lt_id = 0x1FFFFFFE;
                    iut_id = 0x1FFFFFFF;
                    break;
                case 6:
                    lt_id_type = IdentifierType::Extended;
                    iut_id_type = IdentifierType::Extended;
                    iut_rtr_flag = RtrFlag::RtrFrame;
                    lt_id = rand() % (int)pow(2, 29);
                    iut_id = lt_id;
                    break;
                default:
                    break;
                }
            } else if (test_variant == TestVariant::CanFdEnabled) {
                switch (elem_test.index_)
                {
                case 1:
                    lt_frame_type = FrameType::Can2_0;
                    iut_frame_type = FrameType::CanFd;
                    lt_id_type = IdentifierType::Base;
                    iut_id_type = IdentifierType::Base;
                    lt_id = 0x3FE;
                    iut_id = 0x3FF;
                    break;
                case 2:
                    lt_frame_type = FrameType::CanFd;
                    iut_frame_type = FrameType::Can2_0;
                    lt_id_type = IdentifierType::Base;
                    iut_id_type = IdentifierType::Base;
                    lt_id = rand() % (int)pow(2, 11);
                    iut_id = lt_id;
                    iut_rtr_flag = RtrFlag::RtrFrame;
                    break;
                case 3:
                    lt_frame_type = FrameType::Can2_0;
                    iut_frame_type = FrameType::CanFd;
                    lt_id_type = IdentifierType::Extended;
                    iut_id_type = IdentifierType::Extended;
                    lt_id = 0x1FFFFFFE;
                    iut_id = 0x1FFFFFFF;
                    break;
                case 4:
                    lt_frame_type = FrameType::CanFd;
                    iut_frame_type = FrameType::Can2_0;
                    lt_id_type = IdentifierType::Extended;
                    iut_id_type = IdentifierType::Extended;
                    iut_rtr_flag = RtrFlag::RtrFrame;
                    lt_id = rand() % (int)pow(2, 29);
                    iut_id = lt_id;
                    break;
                default:
                    break;
                }
            }

            /* For IUT */
            frame_flags = std::make_unique<FrameFlags>(iut_frame_type, iut_id_type, iut_rtr_flag,
                                                        EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x0, iut_id);
            RandomizeAndPrint(golden_frm.get());

            /* For LT */
            frame_flags_2 = std::make_unique<FrameFlags>(lt_frame_type, lt_id_type, lt_rtr_flag,
                                                        EsiFlag::ErrorActive);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, 0x0, lt_id);
            RandomizeAndPrint(golden_frm_2.get());

            /* Driven/monitored is derived from LTs frame since this one wins over IUTs frame! */
            driver_bit_frm = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm = ConvertBitFrame(*golden_frm_2);

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
            Bit *bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::Sof);
            if (test_variant == TestVariant::Common)
            {
                switch (elem_test.index_)
                {
                case 1:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::Rtr);
                    break;
                case 2:
                    /* In IUTs frame SRR is on RTR position of driven frame */
                    bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::Rtr);
                    break;
                case 3:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::Ide);
                    break;
                case 4:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOfNoStuffBits(
                                            10, BitType::BaseIdentifier);
                    break;
                case 5:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOfNoStuffBits(17,
                                        BitType::IdentifierExtension);
                    break;
                case 6:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::Rtr);
                default:
                    break;
                }
            } else if (test_variant == TestVariant::CanFdEnabled) {
                switch (elem_test.index_)
                {
                case 1:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOfNoStuffBits(10,
                                        BitType::BaseIdentifier);
                    break;
                case 2:
                    /* In IUTs frame R1 is on position of RTR */
                    bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::R1);
                    break;
                case 3:
                    bit_to_loose_arb = monitor_bit_frm->GetBitOfNoStuffBits(17,
                                        BitType::IdentifierExtension);
                    break;
                case 4:
                    /* In IUTs frame R1 is on position of RTR */
                    bit_to_loose_arb = monitor_bit_frm->GetBitOf(0, BitType::R1);
                default:
                    break;
                }
            }

            /*
             * In all frames monitored bits shall be equal to driven bits up to point where
             * arbitration is lost!
             */
            bit_to_loose_arb->bit_value_ = BitValue::Recessive;
            monitor_bit_frm->LooseArbitration(bit_to_loose_arb);

            /* Compensate input delay, lenghten bit on which arbitration was lost */
            bit_to_loose_arb->GetLastTimeQuantaIterator(BitPhase::Ph2)->Lengthen(dut_input_delay);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);
            driver_bit_frm_2->TurnReceivedFrame();

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            /* IUTs frame is sent! */
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();
            /* Received should be the frame which is sent by LT */
            CheckRxFrame(*golden_frm_2);

            FreeTestObjects();
            return FinishElementaryTest();
        }

};