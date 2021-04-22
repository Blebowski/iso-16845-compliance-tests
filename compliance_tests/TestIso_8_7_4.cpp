/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 15.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.7.4
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT, acting
 *        as a transmitter, detecting a negative phase error e on a recessive
 *        to dominant edge with |e| ≤ SJW(N).
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 * 
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Phase error e
 *      FDF = 0
 * 
 * Elementary test cases:
 *  There are min{SJW(N), [Phase_Seg2(N) – IPT]} elementary tests to perform
 *  for at least 1 bit rate configuration.
 * 
 *      #1 The values tested for e are measured in time quanta
 *         |e| ∈ {1, min[SJW(N)], [Phase_Seg2(N) – IPT]}.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  The LT shortens a recessive bit preceding a dominant bit by an amount of
 *  |e| inside the arbitration field according to elementary test cases.
 * 
 * Response:
 *  The next edge sent by the IUT occurs an integer number of bit times after
 *  the edge applied by the LT.
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

class TestIso_8_7_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);

            int num_elem_tests = nominal_bit_timing.sjw_;
            if (nominal_bit_timing.ph2_ < nominal_bit_timing.sjw_)
                num_elem_tests = nominal_bit_timing.ph2_;

            for (int i = 0; i < num_elem_tests; i++)
            {
                ElementaryTest test = ElementaryTest(i + 1);
                test.e = i + 1;
                AddElemTest(TestVariant::Common, std::move(test));
            }

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(10));
            CanAgentSetWaitForMonitor(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Choose random recessive bit in arbitration field which is followed by dominant
             *      bit.
             *   2. Shorten PH2 of this bit by e. Shorten in both driven and monitored frames.
             *   3. Insert ACK to driven frame.
             * 
             * Note: TX/RX feedback must be disabled, since we modify driven frame.
             *************************************************************************************/
            Bit *bit_to_shorten;
            Bit *next_bit;
            int bit_index;
            do
            {
                bit_to_shorten = driver_bit_frm->GetRandomBitOf(BitType::BaseIdentifier);
                bit_index = driver_bit_frm->GetBitIndex(bit_to_shorten);
                next_bit = driver_bit_frm->GetBit(bit_index + 1);
            } while (!(bit_to_shorten->bit_value_ == BitValue::Recessive &&
                        next_bit->bit_value_ == BitValue::Dominant));

            bit_to_shorten->ShortenPhase(BitPhase::Ph2, elem_test.e);
            monitor_bit_frm->GetBit(bit_index)->ShortenPhase(BitPhase::Ph2, elem_test.e);

            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

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

            return FinishElementaryTest();
        }

};