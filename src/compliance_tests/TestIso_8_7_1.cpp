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
 * @date 28.9.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.7.1
 *
 * @brief The purpose of this test is to verify the sample point of an IUT
 *        acting as a transmitter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 *
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *  FDF = 0
 *
 * Elementary test cases:
 *   Test each possible sampling point for at least 1 bit rate configuration.
 *   Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame according to
 *  elementary test cases.
 *  The LT forces a dominant bit preceded by a recessive bit from one
 *  minimum time quantum after the sampling point up to end of bit to
 *  recessive value.
 *  Later, the LT forces another dominant bit preceded by a recessive bit from
 *  sampling point – 1 TQ(N) – minimum time quantum up to end of bit to
 *  recessive value.
 *
 * Response:
 *  The IUT shall generate an error frame on the bit position following the
 *  second shortened bit.
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

class TestIso_8_7_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTestForEachSamplePoint(TestVariant::Common, true, FrameType::Can2_0);
            SetupMonitorTxTests();

            assert((nominal_bit_timing.brp_ > 1 &&
                    "BRP Nominal must be bigger than 1 in this test due to test architecture!"));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, true);
            ReconfigureDutBitTiming();
            WaitDutErrorActive();

            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
                            RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame (TX/RX feedback disabled)
             *   2. Force 2-nd bit of data field (dominant preceeded by recessive), to recessive
             *      from one time quanta after sample point in driven frame.
             *   3. In second frame, force 2-nd bit of data field (same as before) to recessive
             *      from one time quantum before sample point till the end of frame to recessive.
             *      Also force last cycle of previous time quanta. This accounts for "minimal time
             *      quanta" subtraction!
             *   4. Insert Expected Error frame from next bit of data field. Insert to both driven
             *      and monitored frame since TX/RX feebdack is disabled!
             *   5. Append second frame to first frame.
             *   6. Create next frame which is the same as first frame, but no values are forced.
             *      Put ACK low.
             *   7. Append frame from point 6 to test frame. This frame represents IUTs retransmi-
             *      ssion due to error detected in previous frame.
             *************************************************************************************/
            driver_bit_frm->PutAcknowledge(dut_input_delay);

            Bit *bit_to_corrupt = driver_bit_frm->GetBitOf(1, BitType::Data);
            int start_index = nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ + 2;
            int end_index = bit_to_corrupt->GetLengthTimeQuanta();
            bit_to_corrupt->ForceTimeQuanta(start_index, end_index, BitValue::Recessive);

            bit_to_corrupt = driver_bit_frm_2->GetBitOf(1, BitType::Data);
            start_index = nominal_bit_timing.prop_ + nominal_bit_timing.ph1_;
            bit_to_corrupt->ForceTimeQuanta(start_index, end_index, BitValue::Recessive);

            int cycles_length = bit_to_corrupt->GetTimeQuanta(start_index - 1)->getLengthCycles();
            bit_to_corrupt->GetTimeQuanta(start_index - 1)->ForceCycleValue(cycles_length - 1,
                BitValue::Recessive);

            driver_bit_frm_2->InsertActiveErrorFrame(2, BitType::Data);
            monitor_bit_frm_2->InsertActiveErrorFrame(2, BitType::Data);

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);
            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            tec_old = dut_ifc->GetTec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();

            CheckLowerTesterResult();

            return FinishElementaryTest();
        }

};