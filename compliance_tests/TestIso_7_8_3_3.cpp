/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 23.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.3.3
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT dete-
 *        cting a positive phase error e on a recessive to dominant edge with
 *        e ≤ SJW(D) on bit position CRC delimiter.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          CRC: LSB = 1
 *          CRC delimiter
 *          FDF = 1
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e
 *      for at least 1 bit rate configuration.
 *          #1 The values tested for e are measured in time quanta with
 *             e ∈ [1, SJW(D)].
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a test frame with a recessive bit value at last bit of CRC.
 *  The LT forces the CRC delimiter to dominant bit value.
 *  Then, the recessive to dominant edge between LSB of CRC and CRC delimiter
 *  shall be delayed by additional e TQ(D)’s of recessive value at the
 *  beginning of CRC delimiter bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed CRC delimiter bit to
 *  recessive. This recessive part of Phase_seg2 start at e − 1 TQ(D) after
 *  sampling point.
 * 
 * Response:
 *  The modified CRC delimiter bit shall be sampled as recessive.
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

class TestIso_7_8_3_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (size_t i = 1; i <= data_bit_timing.sjw_; i++)
            {
                ElementaryTest test = ElementaryTest(i);
                test.e = i;
                AddElemTest(TestVariant::CanFdEnabled, std::move(test));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x55;
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, IdentifierType::Base,
                                                       RtrFlag::DataFrame, BrsFlag::Shift,
                                                       EsiFlag::ErrorActive);
            // Frame was empirically debugged to have last bit of CRC in 1!
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 50, &data_byte);
            golden_frm->Print();

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force CRC delimiter of driven frame to dominant.
             *   3. Force first e bits of CRC delimiter to Recessive (This delays dominant to
             *      recessive edge by e T!)
             *   4. Shorten PH2 of CRC Delimiter to 0 since this-one is in multiples of nominal
             *      Time quanta. Lengthen PH1 (still in data time quanta), by e - 1. This has equal
             *      effect as forcing the bit to Recessive e - 1 after sample point. Next bit is
             *      ACK which is transmitted recessive by driver.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *crc_delimiter = driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter);
            crc_delimiter->bit_value_ = BitValue::Dominant;

            for (int j = 0; j < elem_test.e; j++)
                crc_delimiter->ForceTimeQuanta(j, BitValue::Recessive);

            crc_delimiter->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            BitPhase phase = crc_delimiter->PrevBitPhase(BitPhase::Ph2);
            crc_delimiter->LengthenPhase(phase, elem_test.e - 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing CRC Delimiter positive resynchronisation with phase error: %d",
                        elem_test.e);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};