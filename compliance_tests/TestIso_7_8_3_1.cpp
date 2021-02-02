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
 * @test ISO16845 7.8.3.1
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e ≤ SJW(D) on bit position ESI.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          ESI = 1
 *          FDF = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             e ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The first e TQ(D) (e according to elementary test cases) of the ESI bit
 *  are set to recessive, then the following {Prop_Seg(D) + Phase_Seg1(D)]
 *  TQ(D)’s are set to dominant. The rest of the ESI bit, Phase_Seg2(D)+1 is
 *  set to recessive. At all, ESI is lengthened by e TQ(D).
 * 
 * Response:
 *  The modified ESI bit shall be sampled as recessive.
 *  The frame is valid. DontShift error flag shall occur.
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

class TestIso_7_8_3_1 : public test_lib::TestBase
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

            assert(nominal_bit_timing.brp_ == data_bit_timing.brp_ &&
                   "TQ(N) shall equal TQ(D) for this test due to test architecture!");
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift,
                                                        EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);  

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Lengthen SYNC phase of ESI by e (both driven and monitored frame).
             *   3. Force Prop + PH1 TQ after initial e TQ to dominant!
             **************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *esi_bit_driver = driver_bit_frm->GetBitOf(0, BitType::Esi);
            Bit *esi_bit_monitor = monitor_bit_frm->GetBitOf(0, BitType::Esi);

            esi_bit_driver->LengthenPhase(BitPhase::Sync, elem_test.e);
            esi_bit_monitor->LengthenPhase(BitPhase::Sync, elem_test.e);

            for (size_t j = 0; j < data_bit_timing.prop_ + data_bit_timing.ph1_; j++)
                esi_bit_driver->ForceTimeQuanta(elem_test.e + j, BitValue::Dominant);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ESI positive resynchronisation with phase error: %d", elem_test.e);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};