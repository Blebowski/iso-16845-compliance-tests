/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 2.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.1
 * 
 * @brief The purpose of this test is to verify the position of the sample point
 *        of an IUT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform.
 *          Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT shortens a dominant stuff bit in arbitration field by an amount of 
 *  Phase_Seg2(N) and then later shortens another dominant stuff bit by an
 *  amount of [Phase_Seg2(N) + 1] according to elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an error frame on the bit position following the
 *  second shortened stuff bit.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_7_7_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));

            CanAgentConfigureTxToRxFeedback(true);
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

                    frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                                                               IdentifierType::Base);

                    /* Base ID full of 1s */
                    int id = pow(2,11) - 1;
                    golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, id);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Monitor frame as if received.
                     *   2. Shorten 6-th bit (1st stuff bit) of driven frame by PhaseSeg2.
                     *   3. Shorten 12-th bit (2nd stuff bit) of driven frame still
                     *      in Base ID by PhaseSeg2 + 1.
                     *   4. Correct lenght of one of the monitored bits since second
                     *      stuff bit causes negative re-synchronization.
                     *****************************************************************************/
                    monitor_bit_frm->TurnReceivedFrame();

                    Bit *first_stuff_bit = driver_bit_frm->GetStuffBit(0);
                    first_stuff_bit->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);

                    Bit *second_stuff_bit = driver_bit_frm->GetStuffBit(1);
                    second_stuff_bit->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
                    BitPhase previous_phase = second_stuff_bit->PrevBitPhase(BitPhase::Ph2);
                    second_stuff_bit->ShortenPhase(previous_phase, 1);

                    /* Compensate the monitored frame as if negative resynchronisation
                     * happend
                     */
                    size_t resync_amount = nominal_bit_timing.ph2_;
                    if (nominal_bit_timing.sjw_ < resync_amount)
                        resync_amount = nominal_bit_timing.sjw_;
                    monitor_bit_frm->GetBitOf(11, BitType::BaseIdentifier)->ShortenPhase(
                        BitPhase::Ph2, resync_amount);

                    /* 5 + Stuff + 5 + Stuff = 12 bits. We need to insert error frame
                    from 13-th bit on! */
                    int bit_index = driver_bit_frm->GetBitIndex(
                                    driver_bit_frm->GetBitOf(12, BitType::BaseIdentifier));

                    /* Expected error frame on monitor (from start of bit after stuff bit)
                     * Driver will have passive error frame -> transmitt all recessive */
                    monitor_bit_frm->InsertActiveErrorFrame(bit_index);
                    driver_bit_frm->InsertPassiveErrorFrame(bit_index);

                    driver_bit_frm->Print(true);
                    monitor_bit_frm->Print(true);

                    /***************************************************************************** 
                     * Execute test
                     *****************************************************************************/
                    PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                    RunLowerTester(true, true);
                    CheckLowerTesterResult();
                }
            }
            return (int)FinishTest();
        }
};