/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 21.12.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.2.2
 * 
 * @brief The purpose of this test is to verify the secondary sample point of
 *        an IUT will not be applied on bit position BRS if the IUT acts as a
 *        transmitter with a delay, d, between transmitted and received signal.
 * @version CAN FD enabled
 * 
 * Test variables:
 *      Available configuration methods for delay compensation = fix programmed
 *      or automatically measured.
 *          Delay, d, in range of TQ (D) = d ∈ (1, 2 data bit times)
 *          “res” bit
 *          BRS = 1
 *          FDF = 1
 * 
 * Elementary test cases:
 *  There are two elementary tests to perform for 1 bit rate configuration and
 *  each way of configuration of delay compensation - fix programmed or
 *  automatically measured, shall be checked.
 *      #1 d = 1 data bit times
 *      #2 d = 2 data bit times
 *
 *  Test for late Sampling_Point(N):
 *      bit level changed after sampling point to wrong value.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation shall be enabled. SSP offset shall be
 *  configured to evaluate the delayed bit on similar position like the
 *  sampling point in data phase [Sampling_Point(D)].
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame with recessive BRS bit.
 *  The LT prolonged the SOF bit on IUT receive input by an amount of d
 *  according to elementary test cases to shift the IUT received sequence
 *  relative against the transmitted sequence of IUT.
 * 
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The frame is invalid. An error flag shall occur.
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

class TestIso_8_8_2_2 : public test_lib::TestBase
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

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
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

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Delay received sequence by d data bit times:
             *          Elem test 1,2 : d = 1
             *          Elem test 3,4 : d = 2
             *      This is done by prolonging SOF of driven frame.
             *   2. Force BRS in shifted frame to dominant for Sync + Prop + PH1 - d. Note that d
             *      is measured in cycles, not time quantas, therefore we must count cycle by cycle!
             *   3. Insert Active Error frame to monitored and driven frames from ESI bit!
             *   4. Append retransmitted frame by IUT!
             *************************************************************************************/
            int d = data_bit_timing.GetBitLengthCycles();
            if (elem_test.index == 3 || elem_test.index == 4)
                d *= 2;
            driver_bit_frm->GetBit(0)->GetTimeQuanta(0)->Lengthen(d);

            size_t force_cycles = nominal_bit_timing.brp_ *
                (nominal_bit_timing.prop_ + nominal_bit_timing.ph1_ + 1) - d;

            Bit *brs = driver_bit_frm->GetBitOf(0, BitType::Brs);
            auto tq = brs->GetTimeQuantaIterator(0);
            do
            {
                if (force_cycles > tq->getLengthCycles())
                {
                    tq->ForceValue(BitValue::Dominant);
                    force_cycles -= tq->getLengthCycles();
                } else {
                    for (size_t i = 0; i < force_cycles; i++)
                        tq->ForceCycleValue(i, BitValue::Dominant);
                    force_cycles = 0;
                }
                tq++;
            } while (force_cycles > 0);

            driver_bit_frm->InsertActiveErrorFrame(0, BitType::Esi);
            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Esi);

            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/

            /* Reconfigure SSP: Test 1, 3 -> Measured + Offset, Test 2, 4 -> Offset only */
            dut_ifc->Disable();
            if (elem_test.index == 1 || elem_test.index == 3)
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