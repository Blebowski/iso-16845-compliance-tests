/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 16.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.5.3
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a negative phase error e on a recessive to dominant edge with
 *        |e| ≤ SJW on bit position ACK.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *          Phase error e
 *          ACK
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *
 *          #1 The values tested for e are measured in time quanta where:
 *              |e| ∈ [1, SJW(N)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame.
 *  The LT shortened the CRC delimiter by an amount of |e| TQ according to
 *  elementary test cases.
 *  Additionally, the Phase_Seg2(N) of this dominant ACK bit shall be forced
 *  to recessive.
 *
 * Response:
 *  The modified ACK bit shall be sampled as dominant.
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

class TestIso_7_8_5_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = 1; i <= data_bit_timing.sjw_; i++)
            {
                ElementaryTest test = ElementaryTest(i);
                test.e_ = i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }
            
            // Note: We cant enable TX to RX feedback here since DUT would
            //       screw us modified bits by transmitting dominant ACK!
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force driven ACK bit to Dominant.
             *   3. Shorten CRC delimiter of driven and monitored bits by e.
             *   4. Force Phase 2 of ACK to Recessive on driven bit!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *crc_delimiter_driver = driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter);
            Bit *crc_delimiter_monitor = monitor_bit_frm->GetBitOf(0, BitType::CrcDelimiter);
            Bit *ack_driver = driver_bit_frm->GetBitOf(0, BitType::Ack);

            crc_delimiter_driver->ShortenPhase(BitPhase::Ph2, elem_test.e_);
            crc_delimiter_monitor->ShortenPhase(BitPhase::Ph2, elem_test.e_);

            for (size_t j = 0; j < nominal_bit_timing.ph2_; j++)
                ack_driver->ForceTimeQuanta(j, BitPhase::Ph2, BitValue::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing ACK negative resynchronisation with phase error: %d",
                        elem_test.e_);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
};