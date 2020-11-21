/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 17.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.7.5
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT acting
 *        as a transmitter detecting a negative phase error e on a recessive
 *        to dominant bit with |e| > SJW(N).
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
 *  There are {[Phase_Seg2(N) – IPT] – SJW(N)} elementary tests to perform
 *  for at least 1 bit rate configuration.
 *      #1 The values tested for e are measured in time quanta
 *         |e| ∈ {[SJW(N) + 1], [Phase_Seg2(N) – IPT]}.
 *  
 *   Refer to 6.2.3.
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
 *  The next edge sent by the IUT occurs an integer number of bit times plus
 *  an amount [|e| – SJW(N)] after the edge applied by the LT.
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

class TestIso_8_7_5 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);

            int num_elem_tests = nominal_bit_timing.ph2_ - nominal_bit_timing.sjw_;

            for (int i = 0; i < num_elem_tests; i++)
            {
                ElementaryTest test = ElementaryTest(i + 1);
                test.e = nominal_bit_timing.sjw_ + i + 1;
                elem_tests[0].push_back(test);
            }

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetWaitForMonitor(true);
        }

        int Run()
        {
            SetupTestEnvironment();

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                    EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Choose random recessive bit in arbitration field, which is followed by
                     *      dominant bit.
                     *   2. Shorten PH2 of this bit by SJW. Shorten in both driven and monitored
                     *      frames. This corresponds to by how much IUT shall resynchronize.
                     *      Dont shorten by whole e, because that would leave some remaining phase
                     *      error for next bits, and next bits would need to be compensated!
                     *   3. Force last e - SJW TQs of PH2 of this bit in driven frame to Dominant.
                     *      This effectively achieves as if bit was shortened by whole e, but next
                     *      bit is prolonged by e - SJW. Therefore, the position of next edge will
                     *      be driven in SYNC, and at the same place will be position of monitored
                     *      edge.
                     *   4. Insert ACK to driven frame.
                     * 
                     * Note: TX/RX feedback must be disabled, since we modify driven frame.
                     *****************************************************************************/
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

                    bit_to_shorten->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);
                    monitor_bit_frm->GetBit(bit_index)->ShortenPhase(BitPhase::Ph2,
                        nominal_bit_timing.sjw_);

                    int phase_2_len = bit_to_shorten->GetPhaseLenTimeQuanta(BitPhase::Ph2);
                    for (size_t i = 0; i < elem_test.e - nominal_bit_timing.sjw_; i++)
                        bit_to_shorten->ForceTimeQuanta(phase_2_len - i - 1, BitPhase::Ph2,
                                                        BitValue::Dominant);

                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    StartDriverAndMonitor();
                    dut_ifc->SendFrame(golden_frm.get());
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();
                }
            }
            return (int)FinishTest();
        }
};