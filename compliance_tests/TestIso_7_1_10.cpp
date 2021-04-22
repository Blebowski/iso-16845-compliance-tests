/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 19.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.10
 * 
 * @brief This test verifies the behaviour of the IUT when receiving a correct
 *        base format frame with particular data containing critical stuffing
 *        bit profiles in the different fields of the frame according to test
 *        variables.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *      Classical CAN
 *          ID, RTR, FDF, DLC, DATA
 * 
 *      CAN FD Tolerant, CAN FD Enabled
 *          ID, RTR, DLC, DATA
 *
 *      CAN FD Enabled
 *          ID, RRS, BRS, ESI, DLC, DATA
 * 
 * Elementary test cases:
 *                               Classical CAN
 *              ID          CTRL                DATA
 *      #1     0x78         0x08              0x01, all others 0xE1
 *      #2    0x41F         0x01              0x00
 *      #3    0x707         0x1F              all bytes 0x0F
 *      #4    0x360         0x10                -
 *      #5    0x730         0x10                -
 *      #6    0x47F         0x01              0x1F
 *      #7    0x758         0x00                -
 *      #8    0x777         0x01              0x1F
 *      #9    0x7EF         0x42                -
 *     #10    0x3EA         0x5F                -
 * 
 *                      CAN FD Tolerant, CAN FD Enabled
 *              ID          CTRL                DATA
 *      #1     0x78         0x08              0x01, all others 0xE1
 *      #2    0x41F         0x01              0x00
 *      #3    0x707         0x0F              all bytes 0x0F
 *      #4    0x360         0x00                -
 *      #5    0x730         0x00                -
 *      #6    0x47F         0x01              0x1F
 *      #7    0x758         0x00                -
 *      #8    0x777         0x01              0x1F
 *      #9    0x7EF         0x42                -
 *     #10    0x3EA         0x4F                -
 * 
 *                              CAN FD Enabled
 *              ID          CTRL                DATA
 *      #1     0x78         0xAE              0xF8, all others 0x78
 *      #2    0x47C         0xA8              all bytes 0x3C
 *      #3    0x41E         0xBE              all bytes 0x1E
 *      #4    0x20F         0x9F              all bytes 0x0F
 *      #5    0x107        0x28F              all bytes 0x87
 *      #6    0x7C3         0x83              all bytes 0xC3
 *      #7    0x3E1         0xA3              all bytes 0xE1
 *      #8    0x1F0         0xA1              0xF0
 *      #9    0x000         0xA0                -
 *     #10    0x7FF         0xB0                -
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for each of the elementary tests.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "../vpi_lib/vpiComplianceLib.hpp"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"
#include "../test_lib/TestLoader.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;
using namespace test_lib;

class TestIso_7_1_10 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::ClassicalFdCommon);
            for (auto const &test_variant : test_variants)
                for (size_t j = 0; j < 10; j++)
                    AddElemTest(test_variant, ElementaryTest(j + 1));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            int id = 0;
            uint8_t dlc = 0;
            uint8_t data[64] = {};

            /* Variants differ only in value of reserved bit! CAN 2.0 shall accept FDF recessive
             * and CAN FD Tolerant shall go to protocol exception!
             */
            if (test_variant == TestVariant::Can_2_0 || test_variant == TestVariant::CanFdTolerant)
            {
                switch (elem_test.index)
                {
                case 1:
                    id = 0x78;
                    dlc = 0x8;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x01;
                    for (int i = 1; i < 8; i++)
                        data[i] = 0xE1;
                    break;

                case 2:
                    id = 0x41F;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x00;
                    break;

                case 3:
                    id = 0x707;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    for (int i = 0; i < 8; i++)
                        data[i] = 0x0F;
                    break;

                case 4:
                    id = 0x360;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 5:
                    id = 0x730;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 6:
                    id = 0x47F;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x1F;
                    break;

                case 7:
                    id = 0x758;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    break;

                case 8:
                    id = 0x777;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                    data[0] = 0x1F;
                    break;

                case 9:
                    id = 0x7EF;
                    dlc = 0x2;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::RtrFrame);
                    break;

                case 10:
                    id = 0x3EA;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Base, RtrFlag::RtrFrame);
                    break;

                default:
                    TestMessage("Invalid Elementary test index: %d", elem_test.index);
                    break;
                }

            }
            else if (test_variant == TestVariant::CanFdEnabled)
            {
                switch (elem_test.index)
                {
                case 1:
                    id = 0x78;
                    dlc = 0xE;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    data[0] = 0x01;
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xE1;
                    break;

                case 2:
                    id = 0x47C;
                    dlc = 0x8;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x3C;
                    break;

                case 3:
                    id = 0x41E;
                    dlc = 0xE;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorPassive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x1E;
                    break;

                case 4:
                    id = 0x20F;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorPassive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x0F;
                    break;

                case 5:
                    id = 0x107;
                    dlc = 0xF;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorActive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0x87;
                    break;

                case 6:
                    id = 0x7C3;
                    dlc = 0x3;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorActive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xC3;
                    break;

                case 7:
                    id = 0x3E1;
                    dlc = 0x3;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xE1;
                    break;

                case 8:
                    id = 0x1F0;
                    dlc = 0x1;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    for (int i = 1; i < 64; i++)
                        data[i] = 0xF0;
                    break;

                case 9:
                    id = 0x000;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                    break;

                case 10:
                    id = 0x7FF;
                    dlc = 0x0;
                    frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorPassive);
                    break;

                default:
                    break;
                }
            }

            golden_frm = std::make_unique<Frame>(*frame_flags, dlc, id, data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Modify some of the bits as per elementary test cases
             *   2. Update the frame since number of stuff bits might have changed.
             *   3. Turn monitored frame to received.
             *************************************************************************************/
            if (test_variant == TestVariant::Can_2_0)
            {
                switch(elem_test.index)
                {
                case 3:
                case 4:
                case 5:
                case 10:
                    driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                default:
                    break;
                }
            }
            else if (test_variant == TestVariant::CanFdEnabled)
            {
                if (elem_test.index == 5)
                {
                    driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                }
            }

            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            monitor_bit_frm->TurnReceivedFrame();

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};
