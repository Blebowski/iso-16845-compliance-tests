/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 1.1.2021
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.3.2
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
 *      #1 Recessive to dominant edge with |e| = SJW(D) in DATA bit.
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
 *  of a recessive bit to dominant according to elementary test cases.
 *  The LT forces a following recessive bit to dominant for
 *      [Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) − 1TQ(D)].
 * 
 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The frame is valid. No error flag shall occur
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

class TestIso_8_8_3_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);

            ElementaryTest test = ElementaryTest(1);
            test.e = data_bit_timing.sjw_;
            AddElemTest(TestVariant::CanFdEnabled, std::move(test));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);

            assert(data_bit_timing.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, RtrFlag::DataFrame,
                                                        BrsFlag::Shift, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0xF);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Pick random recessive bit in data field which is followed by dominant bit.
             *   3. Force last e TQs of picked bit to dominant.
             *   4. Force first Prop + Ph1 TQs of next bit to dominant.
             *************************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *random_bit;
            Bit *next_bit;
            do {
                random_bit = driver_bit_frm->GetRandomBitOf(BitType::Data);
                int bit_index = driver_bit_frm->GetBitIndex(random_bit);
                next_bit = driver_bit_frm->GetBit(bit_index + 1);
            } while (! (random_bit->bit_value_ == BitValue::Recessive &&
                        next_bit->bit_value_ == BitValue::Recessive));

            for (int i = 0; i < elem_test.e; i++)
                random_bit->ForceTimeQuanta(data_bit_timing.ph2_ - 1 - i,
                                            BitPhase::Ph2, BitValue::Dominant);

            for (size_t i = 0; i < data_bit_timing.prop_ + data_bit_timing.ph1_; i++)
                next_bit->ForceTimeQuanta(i, BitValue::Dominant);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            FreeTestObjects();
            return FinishElementaryTest();
        }
};