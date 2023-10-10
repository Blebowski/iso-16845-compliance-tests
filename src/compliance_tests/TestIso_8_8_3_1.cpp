/***************************************************************************** 
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
 * @date 1.1.2021
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.3.1
 * 
 * @brief The purpose of this test is to verify that the behaviour of an IUT,
 *        acting as a transmitter, will not react to a negative phase error e
 *        on a recessive to dominant edge with |e| ≤ SJW(D) in data phase.
 * @version CAN FD enabled
 * 
 * Test variables:
 *  CAN FD enabled
 * 
 *  Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Phase error e
 *      BRS = 1
 *      ESI = 1
 *      FDF = 1
 * 
 * Elementary test cases:
 *  There is one elementary test to perform for each possible value of e for
 *  at least 1 bit rate configuration.
 *      #1 Recessive to dominant edge with |e| = SJW(D) in BRS bit.
 *  Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation is disabled
 *  The LT force the IUT to passive state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces e TQ of Phase_Seg2(D) from end of bit toward sampling point
 *  of BRS bit to dominant according to elementary test cases.
 *  The LT forces the ESI bit to dominant for [Sync_Seg(D) + Prop_Seg(D) +
 *  Phase_Seg1(D) − 1 TQ(D)].
 * 
 * Response:
 *  The modified ESI bit shall be sampled as recessive.
 *  The frame is valid. No error flag shall occur
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

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

class TestIso_8_8_3_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);

            ElementaryTest test = ElementaryTest(1);
            test.e_ = data_bit_timing.sjw_;
            AddElemTest(TestVariant::CanFdEnabled, std::move(test));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            
            SetupMonitorTxTests();

            assert(data_bit_timing.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift,
                                                        EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Force last e TQs of BRS to dominant.
             *   3. Force first Prop + Ph1 TQs of ESI to dominant.
             *   4. Append suspend transmission.
             *************************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *brs = driver_bit_frm->GetBitOf(0, BitType::Brs);
            for (int i = 0; i < elem_test.e_; i++)
                brs->ForceTimeQuanta(data_bit_timing.ph2_ - 1 - i, BitPhase::Ph2, BitValue::Dominant);

            Bit *esi = driver_bit_frm->GetBitOf(0, BitType::Esi);
            for (size_t i = 0; i < data_bit_timing.prop_ + data_bit_timing.ph1_; i++)
                esi->ForceTimeQuanta(i, BitValue::Dominant);

            driver_bit_frm->AppendSuspendTransmission();
            monitor_bit_frm->AppendSuspendTransmission();

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(150); /* To make sure IUT is error passive */
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
};