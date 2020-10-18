/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 12.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.3
 * 
 * @brief This test verifies that the IUT detects a stuff error whenever it
 *        receives 6 consecutive bits of the same value until the position of
 *        the CRC delimiter in an extended frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *      Classical CAN
 *          ID, SRR, ID Extension, RTR, FDF, R0, DLC, DATA
 * 
 *      CAN FD Tolerant, CAN FD Enabled
 *          ID, SRR, ID Extension, RTR, FDF = 0, DLC, DATA
 *
 *      CAN FD Enabled
 *          ID, SRR,  ID Extension, RRS, BRS, ESI, DLC, DATA, FDF=1
 *          Data Byte 0 defined all others 0x55
 * 
 * Elementary test cases:
 *                          Classical CAN
 *          ID          CTRL                DATA
 *  #1  0x07C30F0F     0x188                all bytes 0x3C
 *  #2  0x07C0F0F0     0x181                0x00
 *  #3  0x01E31717     0x19F                all bytes 0x0F
 *  #4  0x01E00FF0     0x1BC                0x1F 0x0F 0xE0 0xF0 0x7F 0xE0 0xFF 0x20
 *  #5  0x1FB80000     0x181                0xA0
 *  #6  0x00BC540F     0x1E0                -
 *  #7  0x155D5557     0x1FF                -
 *  #8  0x00000000     0x181                -
 * 
 *                  CAN FD Tolerant, CAN FD Enabled
 *          ID          CTRL                DATA
 *  #1  0x07C30F0F     0x188                all bytes 0x3C
 *  #2  0x07C0F0F0     0x181                0x00
 *  #3  0x01E31717     0x19F                all bytes 0x0F
 *  #4  0x01E00FF0     0x19C                0x1F 0x0F 0xE0 0xF0 0x7F 0xE0 0xFF 0x20
 *  #5  0x1FB80000     0x181                0xA0
 *  #6  0x00BC540F     0x1C0                -
 *  #7  0x155D5557     0x1DF                -
 *  #8  0x00000000     0x181                -
 *
 *                          CAN FD Enabled
 *  #1  0x01E38787     0x6AE                0xF8, all others 0x78
 *  #2  0x11F3C3C3     0x2A8                all bytes 0x3C
 *  #3  0x1079C1E1     0x6BE                all bytes 0x1E
 *  #4  0x083DF0F0     0x69F                all bytes 0x0F
 *  #5  0x041EF878     0x68F                all bytes 0x87
 *  #6  0x1F0C3C3C     0x683                all bytes 0xC3
 *  #7  0x0F861E1E     0x6A3                all bytes 0xE1
 *  #8  0x07C30F0F     0x6A1                all bytes 0xF0
 *  #9  0x01E38787     0x3A0                -
 * #10  0x11F3C3C3     0x380                -
 * #11  0x00000000     0x6B0                -
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

class TestIso_7_2_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::ClassicalFdCommon);
            for (size_t i = 0; i < test_variants.size(); i++)
            {
                int num_elem_tests;
                if (test_variants[i] == TestVariant::Can_2_0)
                    num_elem_tests = 8;
                if (test_variants[i] == TestVariant::CanFdTolerant)
                    num_elem_tests = 8;
                if (test_variants[i] == TestVariant::CanFdEnabled)
                    num_elem_tests = 11;

                for (int j = 0; j < num_elem_tests; j++)
                    elem_tests[i].push_back(ElementaryTest(j + 1));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int Run()
        {
            SetupTestEnvironment();

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                /** In this test there is no strict matching of elementary test. ISO states that
                 *  each test shall test each stuff bit and a number. Following counter shall
                 *  keep track of how many stuff bits were tested per variant
                 */
                int num_variants_tested = 0;

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    int id;
                    uint8_t dlc;
                    uint8_t data[64] = {};

                    /* Variants differ only in value of reserved bit! CAN 2.0 shall accept FDF
                     * recessive and CAN FD Tolerant shall go to protocol exception!
                     */
                    if (test_variants[test_variant] == TestVariant::Can_2_0 ||
                        test_variants[test_variant] == TestVariant::CanFdTolerant)
                    {
                        switch (elem_test.index)
                        {
                        case 1:
                            id = 0x07C30F0F;
                            dlc = 0x8;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                            IdentifierType::Extended, RtrFlag::DataFrame);
                            for (int i = 1; i < 8; i++)
                                data[i] = 0x3C;
                            break;

                        case 2:
                            id = 0x07C0F0F0;
                            dlc = 0x1;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::DataFrame);
                            data[0] = 0x00;
                            break;

                        case 3:
                            id = 0x01E31717;
                            dlc = 0xF;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::DataFrame);
                            for (int i = 0; i < 8; i++)
                                data[i] = 0x0F;
                            break;

                        case 4:
                            id = 0x01E00FF0;
                            dlc = 0xC;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::DataFrame);
                            data[0] = 0x1F;
                            data[1] = 0x0F;
                            data[2] = 0xE0;
                            data[3] = 0xF0;
                            data[4] = 0x7F;
                            data[5] = 0xE0;
                            data[6] = 0xFF;
                            data[7] = 0x20;
                            break;

                        case 5:
                            id = 0x1FB80000;
                            dlc = 0x1;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::DataFrame);
                            data[0] = 0xA0;
                            break;

                        case 6:
                            id = 0x00BC540F;
                            dlc = 0x0;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::RtrFrame);
                            break;

                        case 7:
                            id = 0x155D5557;
                            dlc = 0xF;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::RtrFrame);
                            break;

                        case 8:
                            id = 0x00000000;
                            dlc = 0x1;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                                    IdentifierType::Extended, RtrFlag::DataFrame);
                            break;

                        default:
                            break;
                        }

                    }
                    else if (test_variants[test_variant] == TestVariant::CanFdEnabled)
                    {
                        switch (elem_test.index)
                        {
                        case 1:
                            id = 0x07C30F0F;
                            dlc = 0xE;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::Shift, EsiFlag::ErrorActive);
                            data[0] = 0xF8;
                            break;

                        case 2:
                            id = 0x11F3C3C3;
                            dlc = 0x8;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::Shift, EsiFlag::ErrorActive);
                            data[0] = 0x3C;
                            break;

                        case 3:
                            id = 0x1079C1E1;
                            dlc = 0xE;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::Shift, EsiFlag::ErrorPassive);
                            data[0] = 0x1E;
                            break;

                        case 4:
                            id = 0x083DF0F0;
                            dlc = 0xF;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::DontShift, EsiFlag::ErrorPassive);
                            data[0] = 0x0F;
                            break;

                        case 5:
                            id = 0x041EF878;
                            dlc = 0xF;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::DontShift, EsiFlag::ErrorActive);
                            data[0] = 0x87;
                            break;

                        case 6:
                            id = 0x1F0C3C3C;
                            dlc = 0x3;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::DontShift, EsiFlag::ErrorActive);
                            data[0] = 0xC3;
                            break;

                        case 7:
                            id = 0x0F861E1E;
                            dlc = 0x3;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::Shift, EsiFlag::ErrorActive);
                            data[0] = 0xE1;
                            break;

                        case 8:
                            id = 0x07C30F0F;
                            dlc = 0x1;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::Shift, EsiFlag::ErrorActive);
                            data[0] = 0xF0;
                            break;

                        case 9:
                            id = 0x01E38787;
                            dlc = 0x0;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::Shift, EsiFlag::ErrorActive);
                            break;

                        case 10:
                            id = 0x11F3C3C3;
                            dlc = 0x0;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::DontShift, EsiFlag::ErrorActive);
                            break;

                        case 11:
                            id = 0x00000000;
                            dlc = 0x0;
                            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                                    IdentifierType::Extended, RtrFlag::DataFrame,
                                                    BrsFlag::DontShift, EsiFlag::ErrorPassive);
                            break;

                        default:
                            break;
                        }

                        for (int i = 1; i < 64; i++)
                            data[i] = 0x55;
                    }

                    golden_frm = std::make_unique<Frame>(*frame_flags, dlc, id, data);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /**********************************************************************************
                     * Modify test frames:
                     *   1. Modify some of the bits as per elementary test cases
                     *   2. Update the frame since number of stuff bits might have changed.
                     *   3. Turn monitored frame to received.
                     *   4. Pick one of the stuff bits within the frame and flip its value.
                     *   5. Insert Active Error frame to monitored frame. Insert Passive Error frame
                     *      to driven frame (TX/RX feedback enabled).
                     **********************************************************************************/
                    if (test_variants[test_variant] == TestVariant::Can_2_0)
                    {
                        switch(elem_test.index)
                        {
                        case 3:
                            driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                            break;

                        case 4:
                            driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                            driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            break;

                        case 6:
                            driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            break;

                        case 7:
                            driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                            driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            break;

                        default:
                            break;
                        }

                    }
                    else if (test_variants[test_variant] == TestVariant::CanFdEnabled)
                    {
                        switch(elem_test.index)
                        {
                        case 2:
                            driver_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                            monitor_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                            break;

                        case 9:
                        case 10:
                            driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                            driver_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                            monitor_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                            break;


                        default:
                            break;
                        }
                    }

                    driver_bit_frm->UpdateFrame();
                    monitor_bit_frm->UpdateFrame();

                    monitor_bit_frm->TurnReceivedFrame();

                    int num_stuff_bits = driver_bit_frm->GetNumStuffBits(StuffBitType::NormalStuffBit);

                    /*****************************************************************************
                     * Execute test
                     ****************************************************************************/
                    for (int stuff_bit = 0; stuff_bit < num_stuff_bits; stuff_bit++)
                    {
                        TestMessage("Testing stuff bit nr: %d", stuff_bit);
                        printf("%d\n", stuff_bit);
                        num_variants_tested++;

                        /* 
                         * Copy frame to second frame so that we dont loose modification of bits.
                         * Corrupt only second one.
                         */
                        driver_bit_frm_2 = std::make_unique<BitFrame>(*driver_bit_frm);
                        monitor_bit_frm_2 = std::make_unique<BitFrame>(*monitor_bit_frm);

                        Bit *stuff_bit_to_flip = driver_bit_frm_2->GetStuffBit(stuff_bit);
                        int bit_index = driver_bit_frm_2->GetBitIndex(stuff_bit_to_flip);
                        stuff_bit_to_flip->FlipBitValue();

                        driver_bit_frm_2->InsertPassiveErrorFrame(bit_index + 1);
                        monitor_bit_frm_2->InsertActiveErrorFrame(bit_index + 1);

                        /* Do the test itself */
                        dut_ifc->SetRec(0);
                        PushFramesToLowerTester(*driver_bit_frm_2, *monitor_bit_frm_2);
                        RunLowerTester(true, true);
                        CheckLowerTesterResult();

                        driver_bit_frm_2.reset();
                        monitor_bit_frm_2.reset();
                    }
                    FreeTestObjects();
                }
                TestBigMessage("Tested %d stuff bits in this variant!", num_variants_tested);
                printf("%d\n", num_variants_tested);
            }

            return (int)FinishTest();
        }
};