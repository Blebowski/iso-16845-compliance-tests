/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 19.12.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.2.1
 * 
 * @brief The purpose of this test is to verify the secondary sample point of
 *        an IUT acting as a transmitter with a delay, d, between transmitted
 *        signal and received signal will not be applied on bit position “res”
 *        bit.
 * @version CAN FD enabled
 * 
 * Test variables:
 *      Available configuration methods for delay compensation = fix programmed
 *      or automatically measured.
 *          Delay, d, in range of TQ (D) = d ∈ (1, 2 data bit times)
 *          “res” bit
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
 *  The LT causes the IUT to transmit a frame.
 *  The LT prolonged the SOF bit on IUT receive input by an amount of d
 *  according to elementary test cases to shift the IUT received sequence
 *  relative against the transmitted sequence of IUT.
 *  The LT forces Phase_Seg2(N) of the transmitted (not shifted) “res”
 *  bit position to recessive.
 * 
 * Response:
 *  The modified “res” bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur.
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

class TestIso_8_8_2_1 : public test_lib::TestBase
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

            // Following constraint is not due model, IUT issues, it is due to principle of the
            // test, we can't avoid it!
            assert(data_bit_timing.GetBitLengthCycles() * 2 <
                   ((nominal_bit_timing.ph1_ + nominal_bit_timing.prop_ + 1) * nominal_bit_timing.brp_) &&
                   " In this test TSEG1(N) > 2 * Bit time(D) due to test architecture!");

            SetupMonitorTxTests();
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
             *   1. Delay received sequence by d data bit times:
             *          Elem test 1,2 : d = 1
             *          Elem test 3,4 : d = 2
             *      This is done by prolonging SOF of driven frame.
             *   2. Driven sequence is now delayed by d. We need to search TQs in driven frame,
             *      which correspond to PH2 of res bit. These shall be forced to recessive. If IUT
             *      is using SSP, it will sample later than regular SP and detect bit error. If it
             *      is using regular SP, it will sample res correctly as dominant just before it
             *      changes to recessive.
             *   3. Insert ACK to driven frame.
             *************************************************************************************/
            int d = data_bit_timing.GetBitLengthCycles();
            if (elem_test.index_ == 3 || elem_test.index_ == 4)
                d *= 2;
            driver_bit_frm->GetBit(0)->GetTimeQuanta(0)->Lengthen(d);

            /* Following way of forcing "original PH2 of res/R0 bit" is shitty preformance-wise!
             * For each cycle of driven PH2 of R0, we search cycle which is "d" cycles back within
             * whole frame. First "MoveCyclesBack", finds TQ and bit, in which 'orig' cycle is
             * (redundantly multiple times), then it iterates back through bit until it moved
             * for d cycles.
             * 
             * I am aware of the shittines of the solution, but I decided to live with it as
             * the penalty is just not so big!
             * Alternative would be to have also bottom->up reference in Bit/TQ/Cycle hierarchy.
             * This would allow moving cycle-by-cycle with iterator accross multiple TQs/bits.
             */
            Bit *r0 = driver_bit_frm->GetBitOf(0, BitType::R0);
            for (size_t i = 0; i < r0->GetPhaseLenTimeQuanta(BitPhase::Ph2); i++)
            {
                TimeQuanta *tq = r0->GetTimeQuanta(BitPhase::Ph2, i);
                for (size_t j = 0; j < tq->getLengthCycles(); j++)
                {
                    CycleBitValue *orig = tq->getCycleBitValue(j);
                    CycleBitValue *to = driver_bit_frm->MoveCyclesBack(orig, d);
                    to->ForceValue(BitValue::Recessive);
                }
            }

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