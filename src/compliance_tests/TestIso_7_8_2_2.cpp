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
 * @test ISO16845 7.8.2.2
 *
 * @brief The purpose of this test is to verify that the IUT makes a hard
 *        synchronization when receiving an early recessive to dominant edge
 *        between FDF and “res” bit by e, where:
 *            e = Phase_Seg2(N)
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) configuration as available by IUT.
 *          SJW(N) = 1
 *          res
 *          FDF = 1
 *          BRS = 0
 *          ESI = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The LT generates a valid frame with shortened FDF bit by an amount
 *             of e = Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 *  The LT sets the last Phase_Seg2(D) TQ of the dominant BRS bit to recessive.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The hard synchronization shall correct the maximum phase error as defined
 *  in ISO 11898-1.
 *  The frame is valid. No error flag shall occur. The bit rate will not switch
 *  for data phase.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "pli_lib.h"

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

class TestIso_7_8_2_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            ElementaryTest test = ElementaryTest(1);
            test.e_ = nominal_bit_timing.ph2_;
            AddElemTest(TestVariant::CanFdEnabled, std::move(test));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::DontShift);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten PH2 of FDF/EDL bit to 0 (both driven and monitored frames since DUT
             *      shall Hard synchronize)
             *   3. Force TSEG2 of BRS to Recessive on driven frame!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *edl_bit_driver = driver_bit_frm->GetBitOf(0, BitType::Edl);
            Bit *edl_bit_monitor = monitor_bit_frm->GetBitOf(0, BitType::Edl);
            Bit *brs_bit = driver_bit_frm->GetBitOf(0, BitType::Brs);

            edl_bit_driver->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            edl_bit_monitor->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);

            for (size_t j = 0; j < data_bit_timing.ph2_; j++)
                brs_bit->GetTimeQuanta(BitPhase::Ph2, j)->ForceValue(BitValue::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing 'res' bit hard-sync with negative phase error");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};