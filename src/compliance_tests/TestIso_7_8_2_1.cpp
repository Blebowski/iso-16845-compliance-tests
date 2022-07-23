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
 * @date 23.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.2.1
 * 
 * @brief The purpose of this test is to verify that the IUT makes a hard
 *        synchronization when receiving a recessive to dominant edge delayed
 *        by e, where:
 *          e ∈ [SJW(N) + 1, NTQ(N) − Phase_Seg2(N) − 1]
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *          “res” bit
 *          FDF = 1
 *          BRS = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The LT generates a valid frame with prolonged FDF bit by an
 *             amount of e ∈ [SJW(N) + 1, NTQ(N) − Phase_Seg2(N) − 1].
 *      
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  The LT sets the first [Prop_Seg(N) + Phase_Seg1(N)] TQ’s of the recessive
 *  BRS bit to dominant.
 * 
 * Response:
 *  The modified BRS bit shall be sampled as recessive.
 *  The hard synchronization shall correct the maximum phase error as defined
 *  in ISO 11898-1.
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

class TestIso_7_8_2_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            size_t num_elem_tests = nominal_bit_timing.GetBitLengthTimeQuanta() -
                                    nominal_bit_timing.ph2_ -
                                    nominal_bit_timing.sjw_ -
                                    1;
            for (size_t i = 1; i <= num_elem_tests; i++)
            {
                ElementaryTest test = ElementaryTest(i);
                test.e_ = nominal_bit_timing.sjw_ + i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);        

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Prolong FDF/EDL bit by e (both driven and monitored frame since DUT shall
             *      execute hard sync).
             *   3. Set first Prop+Phase1 TQ of BRS to Dominant.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *edl_bit_driver = driver_bit_frm->GetBitOf(0, BitType::Edl);
            Bit *edl_bit_monitor = monitor_bit_frm->GetBitOf(0, BitType::Edl);
            Bit *brs_bit = driver_bit_frm->GetBitOf(0, BitType::Brs);

            edl_bit_driver->LengthenPhase(BitPhase::Ph2, elem_test.e_);
            edl_bit_monitor->LengthenPhase(BitPhase::Ph2, elem_test.e_);

            for (size_t j = 0; j < (nominal_bit_timing.ph1_ + nominal_bit_timing.prop_); j++)
                brs_bit->GetTimeQuanta(j)->ForceValue(BitValue::Dominant);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing 'res' bit hard-sync with phase error: %d", elem_test.e_);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};