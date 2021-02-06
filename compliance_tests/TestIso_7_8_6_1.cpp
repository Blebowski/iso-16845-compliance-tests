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
 * @test ISO16845 7.8.6.1
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| > SJW(D) on bit position ESI.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          Phase error e
 *          ESI = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *              |e| ∈ {[SJW(D) + 1], Phase_Seg2(D)}
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame.
 *  The LT forces an amount of |e| TQ from end of Phase_Seg2(D) of BRS bit to
 *  dominant according to elementary test cases. By this, the BRS bit of the
 *  IUT is shortened by an amount of SJW(D).
 * 
 *  Additionally, the Phase_Seg2(D) of ESI bit shall be forced to recessive.
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

class TestIso_7_8_6_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = data_bit_timing.sjw_ + 1; i <= data_bit_timing.ph2_; i++)
            {
                ElementaryTest test = ElementaryTest(i - data_bit_timing.sjw_);
                test.e = i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }
            
            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift,
                                                       EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force e TQ of BRS at the end of bit to dominant in driven frame.
             *   3. Force Phase 2 of ESI to Recessive on driven frame.
             *   
             *   Note: There is no need to compensate monitored BRS bit length, because driver will
             *         drive nominal frame length and DUT will shorten by SJW. Therefore, DUT will
             *         be SJW TQ behind the driven frame and this will be compensated by DUT during
             *         next synchronisations within a frame!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *brs_bit_driver = driver_bit_frm->GetBitOf(0, BitType::Brs);
            Bit *esi_bit = driver_bit_frm->GetBitOf(0, BitType::Esi);

            for (int j = 0; j < elem_test.e; j++)
                brs_bit_driver->ForceTimeQuanta(data_bit_timing.ph2_ - 1 - j, BitPhase::Ph2,
                                                BitValue::Dominant);

            esi_bit->ForceTimeQuanta(0, data_bit_timing.ph2_ - 1, BitPhase::Ph2, BitValue::Recessive);

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
        ENABLE_UNUSED_ARGS
};