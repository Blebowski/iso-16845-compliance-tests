/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 23.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.1.2
 * 
 * @brief The purpose of this test is to verify the position of the sample
 *        point of an IUT on bit position DATA field.
 * 
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *      DATA field
 *      FDF = 1
 * 
 * Elementary test cases:
 *      There are two elementary tests to perform for at least 1 bit rate
 *      configuration:
 *          #1 test for early sampling point: bit level change to recessive
 *             before sampling point;
 *          #2 test for late sampling point: bit level change to recessive
 *             after sampling point.
 *          
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 * 
 *  Test Data #1:
 *      The LT forces a recessive bit to dominant from beginning up to one TQ(D)
 *      before the sampling point.
 * 
 *  Test DATA #2:
 *      The LT forces a dominant bit to recessive for Phase_Seg2(D).
 * 
 * Response:
 *  Test DATA #1:
 *      The modified data bit shall be sampled as recessive.
 *      The frame is valid. DontShift error flag shall occur.
 * 
 *  Test DATA #2:
 *      The modified data bit shall be sampled as dominant.
 *      The frame is valid. DontShift error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_7_8_1_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = 0; i < 2; i++) {
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::Can2_0));
            }
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // To avoid stuff bits in data field.
            uint8_t data_byte_recessive_sampled = 0x55;
            uint8_t data_byte_dominant_sampled = 0x15;

            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);

            // In 2nd iteration dominant bit will be sampled at second bit position of data field!
            // We must expect this in golden frame so that it will be compared correctly with
            // received frame!
            if (elem_test.index_ == 1)
                golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte_recessive_sampled);
            else
                golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte_dominant_sampled);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received.
             *   2. Modify 2nd bit of data field. Since data is 0x55 this bit is recessive. Flip
             *      its TSEG - 1 (elem test 1) or TSEG1 (elem test 2) to dominant.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *data_bit = driver_bit_frm->GetBitOf(1, BitType::Data);
            data_bit->bit_value_ = BitValue::Recessive;

            int dominant_pulse_length;
            if (elem_test.index_ == 1)
                dominant_pulse_length = data_bit_timing.prop_ + data_bit_timing.ph1_;
            else
                dominant_pulse_length = data_bit_timing.prop_ + data_bit_timing.ph1_ + 1;

            for (int j = 0; j < dominant_pulse_length; j++)
                data_bit->ForceTimeQuanta(j, BitValue::Dominant);    

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            if (elem_test.index_ == 1)
                TestMessage("Testing Data bit sampled Recessive");
            else
                TestMessage("Testing Data bit sampled Dominant");

            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);
            
            return FinishElementaryTest();
        }
};