/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.3
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a negative phase error e on a recessive to dominant edge
 *        with |e| ≤ SJW(N) on bit position ACK.
 * 
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *   Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             |e| ∈ [1, SJW(N)].
 *      
 *      Refer to 6.2.3
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a frame.
 *  The LT forces an amount of e TQ from end of CRC delimiter bit to dominant.
 *  Additionally, the ACK bit shall be forced to recessive from end of bit
 *  toward Sampling_Point(N) for Phase_Seg2(N) + e according to elementary
 *  test cases. The bit shall be sampled as dominant.
 * 
 * Response:
 *  The frame is valid, no error flag shall occur.
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

class TestIso_7_7_11 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            for (size_t i = 1; i <= nominal_bit_timing.sjw_; i++)
            {
                ElementaryTest test = ElementaryTest(i);
                test.e = i;
                AddElemTest(TestVariant::Common, std::move(test));
            }
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Shorten PH2 phase of CRC Delimiter by e. Shorten in both driven and monitored
             *      frame since DUT shall re-sync!
             *   3. Force PH2 of ACK to recessive.
             * 
             * Note: This is not exactly sequence as described in ISO, there bits are not shortened
             *       but flipped, but overall effect is the same!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter)
                ->ShortenPhase(BitPhase::Ph2, elem_test.e);
            monitor_bit_frm->GetBitOf(0, BitType::CrcDelimiter)
                ->ShortenPhase(BitPhase::Ph2, elem_test.e);

            Bit *ack = driver_bit_frm->GetBitOf(0, BitType::Ack);
            ack->bit_value_ = BitValue::Dominant;

            ack->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);

            // Shorten monitored ACK by 1 TQ since DUT will re-synchronise with SYNC segment ended!
            Bit *ack_mon = monitor_bit_frm->GetBitOf(0, BitType::Ack);
            ack_mon->ShortenPhase(BitPhase::Ph1, 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ACK negative phase error: %d", elem_test.e);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);
            
            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};