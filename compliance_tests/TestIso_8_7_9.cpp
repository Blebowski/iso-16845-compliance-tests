/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 21.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.7.9
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT, acting
 *        as a transmitter, synchronizing to a recessive to dominant edge after
 *        the sample point, while sending a dominant bit. The edge is caused by
 *        a disturbance of the dominant bit.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 * 
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *  There is one elementary test to perform for each programmable sampling point
 *  inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 LT sends two time quanta recessive state, starting one time quanta
 *         before the sample point of the IUT.
 *
 *  Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  While the IUT sends a dominant bit, the LT sends two time quanta recessive
 *  state, according to elementary test cases.
 * 
 * Response:
 *  The IUT sends an error flag and the next edge sent by the IUT occurs 6
 *  bit times + [Phase_Seg2(N) – SJW(N)] after the recessive to dominant edge
 *  applied by the LT after the sample point of the dominant bit
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

class TestIso_8_7_9 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
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
                     *   1. Choose random dominant bit from driven frame.
                     *   2. Force last time Quanta of phase before PH2 and first time quanta of
                     *      Phase 2 to recessive.
                     *   3. Shorten PH2 of driven and monitored frames by SJW. This correponds to
                     *      by how much IUT should have resynchronized.
                     *   4. Insert Active Error frame from next bit on.
                     *****************************************************************************/
                    Bit *rand_bit = driver_bit_frm->GetRandomBit(BitValue::Dominant);
                    int rand_bit_index = driver_bit_frm->GetBitIndex(rand_bit);

                    rand_bit->ForceTimeQuanta(0, BitPhase::Ph2, BitValue::Recessive);
                    rand_bit->GetLastTimeQuantaIterator(rand_bit->PrevBitPhase(BitPhase::Ph2))
                        ->ForceValue(BitValue::Recessive);

                    rand_bit->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);
                    monitor_bit_frm->GetBit(rand_bit_index)->ShortenPhase(BitPhase::Ph2,
                        nominal_bit_timing.sjw_);

                    driver_bit_frm->InsertActiveErrorFrame(rand_bit_index + 1);
                    monitor_bit_frm->InsertActiveErrorFrame(rand_bit_index + 1);

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