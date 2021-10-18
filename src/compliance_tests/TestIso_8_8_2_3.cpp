/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 30.12.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.2.3
 * 
 * @brief The purpose of this test is to verify the secondary sample point of
 *        an IUT acting as a transmitter with a delay, d, between transmitted
 *        signal and received signal. The test shall be applied on a bit
 *        position at DATA field.
 * @version CAN FD enabled
 * 
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Available configuration methods for delay compensation = fix programmed
 *      or automatically measured.
 *          Delay, d, in range of TQ (D) = d ∈ (1, 2 data bit times)
 *          FDF = 1
 * 
 * Elementary test cases:
 *  There are two elementary tests to perform for 1 bit rate configuration and
 *  each way of configuration of delay compensation - fix programmed or
 *  automatically measured, shall be checked.
 *      #1 d = 1 data bit times
 *      #2 d = 2 data bit times
 *
 *  — Test part 1 for late SSP: bit level changed after secondary sampling
 *    point to wrong value.
 *  — Test part 2 for early SSP: bit level changed before secondary sampling
 *    point to correct value.
 *  Each available way of configuration of delay compensation shall be checked
 *  separately by execution of test #1 to #2.
 *  
 *  Refer to 6.2.3
 * 
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation shall be enabled. SSP offset shall be
 *  configured to evaluate the delayed bit on similar position like the
 *  sampling point in data phase [Sampling_Point(D)].
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT prolonged the SOF bit on IUT receive input by an amount of d
 *  according to elementary to elementary test cases to shift the IUT received
 *  sequence relative against the transmitted sequence of IUT.
 * 
 *  Test DATA part 1:
 *  The LT forces a dominant bit to recessive starting at
 *      [delay compensation + offset + 1TQ(D)] relative to transmitted bit.
 * 
 *  Test DATA part 2:
 *  The LT forces a recessive bit to dominant up to the secondary sampling
 *  point − 1TQ(D). [delay compensation + offset − 1TQ(D)] relative to
 *  transmitted bit.
 * 
 * Response:
 *  Test DATA part 1:
 *      The modified data bit shall be sampled as dominant.
 *      The frame is valid. No error flag shall occur.
 *  Test DATA part 2:
 *      The modified data bit shall be sampled as recessive.
 *      The frame is valid. No error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

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

class TestIso_8_8_2_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);

            /*
             * Test defines only two elementary tests, but each type of SSP shall be tested.
             * We have options: Offset, Offset + Measured. This gives us two options for each
             * elementary test, together 4 tests.
             */
            for (int i = 0; i < 4; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1));

            SetupMonitorTxTests();

            assert(data_bit_timing.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");

            // Following constraint is not due model, IUT issues, it is due to principle of the
            // test, we can't avoid it!
            assert(data_bit_timing.GetBitLengthCycles() * 2 <
                   ((nominal_bit_timing.ph1_ + nominal_bit_timing.prop_ + 1) * nominal_bit_timing.brp_) &&
                   " In this test TSEG1(N) > 2 * Bit time(D) due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, RtrFlag::DataFrame,
                                                       BrsFlag::Shift, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Delay received sequence by d data bit times:
             *          Elem test 1,2 : d = 1
             *          Elem test 3,4 : d = 2
             *      This is done by prolonging SOF of driven frame.
             *   2. Pick random dominant bit in data field and force it to Recessive 1 TQ(D) after
             *      sample point. This corresponds to d + offset (offset is configured to sample
             *      point).
             *   3. Pick random recessive bit in data field, and force it to dominant from start
             *      till sample point - 1 TQ(D).
             *   4. Insert ACK to driven frame!
             *************************************************************************************/
            int d = data_bit_timing.GetBitLengthCycles();
            if (elem_test.index_ == 3 || elem_test.index_ == 4)
                d *= 2;
            driver_bit_frm->GetBit(0)->GetTimeQuanta(0)->Lengthen(d);

            Bit *dominant_bit;
            do {
                dominant_bit = driver_bit_frm->GetRandomBitOf(BitType::Data);
            } while (dominant_bit->bit_value_ != BitValue::Dominant);

            for (size_t i = 1; i < data_bit_timing.ph2_; i++)
                dominant_bit->ForceTimeQuanta(i, BitPhase::Ph2, BitValue::Recessive);

            Bit *recessive_bit;
            do {
                recessive_bit = driver_bit_frm->GetRandomBitOf(BitType::Data);
            } while (recessive_bit->bit_value_ != BitValue::Recessive);

            for (size_t i = 0; i < data_bit_timing.ph1_ + data_bit_timing.prop_; i++)
                recessive_bit->ForceTimeQuanta(i, BitValue::Dominant);

            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/

            /* Reconfigure SSP: Test 1, 3 -> Measured + Offset, Test 2, 4 -> Offset only */
            dut_ifc->Disable();
            if (elem_test.index_ == 1 || elem_test.index_ == 3)
            {
                /* Offset as if normal sample point, TX/RX delay will be measured and added
                 * by IUT. Offset in clock cycles! (minimal time quanta)
                 */
                int ssp_offset = data_bit_timing.brp_ *
                                 (data_bit_timing.prop_ + data_bit_timing.ph1_ + 1);
                dut_ifc->ConfigureSsp(SspType::MeasuredPlusOffset, ssp_offset);
            } else {
                /* We need to incorporate d into the delay! */
                int ssp_offset = data_bit_timing.brp_ *
                                 (data_bit_timing.prop_ + data_bit_timing.ph1_ + 1) + d;
                dut_ifc->ConfigureSsp(SspType::Offset, ssp_offset);
            }
            dut_ifc->Enable();
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(2000);

            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
};