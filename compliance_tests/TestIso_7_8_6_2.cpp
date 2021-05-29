/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 19.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.6.2
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| > SJW(D) on bit position DATA.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          DATA field
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              |e| ∈ {[SJW(D) + 1], Phase_Seg2(D)}
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  The LT forces an amount of |e| TQ from end of Phase_Seg2(D) of the DATA
 *  bit before the dominant stuff bit to dominant according to elementary test
 *  cases. By this, the DATA bit of the IUT is shortened by an amount of SJW(D).
 *  
 *  Additionally, the Phase_Seg2(D) of the dominant stuff bit shall be forced to
 *  recessive.
 *
 * Response:
 *  The modified stuff bit shall be sampled as dominant.
 *  The frame is valid, no error flag shall occur.
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

class TestIso_7_8_6_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = data_bit_timing.sjw_ + 1; i <= data_bit_timing.ph2_; i++)
            {
                ElementaryTest test = ElementaryTest(i - data_bit_timing.sjw_);
                test.e_ = i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }
            
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x7F;
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force last e TQ of 6-th bit of data field by e TQ to dominant. This should be
             *      a bit before stuff bit.
             *   3. Force PH2 of 7-th bit of data field to Recessive. This should be a stuff bit.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *driver_before_stuff_bit = driver_bit_frm->GetBitOf(5, BitType::Data);
            Bit *driver_stuff_bit = driver_bit_frm->GetBitOf(6, BitType::Data);

            for (int j = 0; j < elem_test.e_; j++)
                driver_before_stuff_bit->ForceTimeQuanta(data_bit_timing.ph2_ - 1 - j, BitPhase::Ph2,
                                                         BitValue::Dominant);

            for (size_t j = 0; j < data_bit_timing.ph2_; j++)
                driver_stuff_bit->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing data byte negative resynchronisation with phase error: %d",
                         elem_test.e_);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};