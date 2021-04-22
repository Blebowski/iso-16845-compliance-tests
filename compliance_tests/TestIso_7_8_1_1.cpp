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
 * @test ISO16845 7.8.1.1
 * 
 * @brief The purpose of this test is to verify the position of the sample
 *        point of an IUT on bit position BRS.
 * 
 * @version CAN FD Enabled
 * 
 * Test variables:
 *   Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      BRS
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
 *      Test BRS #1:
 *          The LT forces the BRS bit to dominant from beginning up to one
 *          TQ(N) before Sampling_Point(N).
 *      Test BRS #2:
 *          The LT forces the BRS bit to dominant from beginning up to
 *          Sampling_Point(N).
 * 
 * Response:
 *  Test BRS #1:
 *      The modified BRS bit shall be sampled as recessive.
 *      The frame is valid. No error flag shall occur.
 *  Test BRS #2:
 *      The modified BRS bit shall be sampled as dominant.
 *      The frame is valid. No error flag shall occur. The bit rate will not
 *      switch for data phase.
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

class TestIso_7_8_1_1 : public test_lib::TestBase
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
            // CAN FD frame, Shift/ No shift based on elementary test!
            if (elem_test.index == 1)
                frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);
            else
                frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::DontShift);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x0);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Flip bit value to be sure that forced value has an effect!
             *   3. Force TSEG1 - 1 of BRS to dominant (first elementary test), or TSEG1 of BRS to
             *      dominant (second elementary test).
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_  = BitValue::Dominant;

            Bit *brs = driver_bit_frm->GetBitOf(0, BitType::Brs);

            // For both set the orig. bit value to recessive so that we
            // see the dominant flipped bits!
            brs->bit_value_ = BitValue::Recessive;

            int dominant_pulse_length;
            
            if (elem_test.index == 1)
                dominant_pulse_length = nominal_bit_timing.prop_ + nominal_bit_timing.ph1_;
            else
                dominant_pulse_length = nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ + 1;

            for (int j = 0; j < dominant_pulse_length; j++)
                brs->ForceTimeQuanta(j, BitValue::Dominant);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             **************************************************************************************/
            if (elem_test.index == 1)
                TestMessage("Testing BRS sampled Recessive");
            else
                TestMessage("Testing BRS sampled Dominant");

            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};