/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 24.5.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.8.3.2
 * 
 * @brief The purpose of this test is to verify the behaviour of an IUT
 *        detecting a positive phase error e on a recessive to dominant edge
 *        with e ≤ SJW(D) on bit position DATA.
 *
 * @version CAN FD Enabled
 * 
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *          DATA field
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
 *  The LT sends a frame containing a dominant stuff bit in DATA field.
 *  Then, the recessive to dominant edge before this dominant stuff bit
 *  shall be delayed by additional e TQ(D)’s of recessive value at the
 *  beginning of this stuff bit according to elementary test cases.
 *  The LT forces a part of Phase_Seg2(D) of the delayed stuff bit to
 *  recessive. This recessive part of Phase_seg2 start at e − 1 TQ(D)
 *  after sampling point.
 * 
 * Response:
 *  The modified data bit shall be sampled as recessive.
 *  The wrong value of stuff bit shall cause an error frame.
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

class TestIso_7_8_3_2 : public test_lib::TestBase
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

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x7F; // 7th data bit is dominant stuff bit!
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);
            golden_frm = std::make_unique<Frame>(*frame_flags, 1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Lengthen bit before stuff bit by e in monitored frame! (This covers DUT
             *      re-synchronisation).
             *   3. Force first e TQ D of dominant stuff bit of driven frame to recessive
             *      (this creates phase error of e and shifts sample point by e).
             *   4. Force last PH2 - e - 1 TQ of dominant stuff bit to recessive on driven bit.
             *      Since Recessive value was set to one before sample point (sample point shifted
             *      by e), this shall be bit error!
             *   5. Insert active error frame on monitor from next bit, Insert passive by driver
             *      to send all recessive.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *before_stuff_bit = monitor_bit_frm->GetBitOf(5, BitType::Data);
            before_stuff_bit->LengthenPhase(BitPhase::Ph2, elem_test.e);

            // 7-th bit should be stuff bit
            Bit *driver_stuff_bit = driver_bit_frm->GetBitOf(6, BitType::Data);
            assert(driver_stuff_bit->bit_value_ == BitValue::Dominant);
            int bit_index = driver_bit_frm->GetBitIndex(driver_stuff_bit);
            for (int j = 0; j < elem_test.e; j++)
                driver_stuff_bit->GetTimeQuanta(j)->ForceValue(BitValue::Recessive);

            //driver_stuff_bit->shortenPhase(PH2_PHASE, dataBitTiming.ph2 - i);

            for (size_t j = elem_test.e - 1; j < data_bit_timing.ph2_; j++)
                driver_stuff_bit->GetTimeQuanta(BitPhase::Ph2, j)
                    ->ForceValue(BitValue::Recessive);

            driver_bit_frm->InsertPassiveErrorFrame(bit_index + 2);
            monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("Testing Data byte positive resynchronisation with phase error: %d",
                        elem_test.e);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};