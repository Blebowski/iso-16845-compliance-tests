/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 13.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.1
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW(D) on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          ESI = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *              |e| ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant ESI bit.
 *  The LT shortened the BRS bit by an amount of |e| TQ according to ele-
 *  mentary test cases.
 *  Additionally, the ESI bit shall be forced to recessive value from
 *  [Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) − e] up to end of bit.
 *
 * Response:
 *  The modified ESI bit shall be sampled as dominant.
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

class TestIso_7_8_5_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = 1; i <= data_bit_timing.sjw_; i++)
            {
                ElementaryTest test = ElementaryTest(i);
                test.e = i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift,
                                                       EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *  1. Turn monitor frame as if received!
             *  2. Shorten PH2 of BRS by e.
             *  3. Force ESI to Recessive from Sync+Prop+Phase1-e till the end of bit.
             *     Note: e is negative, therefore this step will actually force Recessive only
             *           in Phase2! This compenstates 'e' of shortening!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *brs_bit = driver_bit_frm->GetBitOf(0, BitType::Brs);
            Bit *esi_bit = driver_bit_frm->GetBitOf(0, BitType::Esi);
            Bit *brs_bit_monitor = monitor_bit_frm->GetBitOf(0, BitType::Brs);

            brs_bit->ShortenPhase(BitPhase::Ph2, elem_test.e);
            brs_bit_monitor->ShortenPhase(BitPhase::Ph2, elem_test.e);

            int start_tq = 1 + data_bit_timing.prop_ + data_bit_timing.ph1_ + elem_test.e;
            for (size_t j = start_tq; j < brs_bit->GetLengthTimeQuanta(); j++)
                esi_bit->ForceTimeQuanta(j, BitValue::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ESI negative resynchronisation with phase error: %d",
                         elem_test.e);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};