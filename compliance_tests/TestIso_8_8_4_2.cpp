/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.1.2021
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.8.4.2
 * 
 * @brief The purpose of this test is to verify that there is no synchroni-
 *        zation within 1 bit time if there are two recessive to dominant
 *        edges between two sample points where the first edge comes before
 *        the synchronization segment.
 * @version CAN FD enabled
 * 
 * Test variables:
 *  CAN FD enabled
 * 
 *  Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      DATA field
 *      FDF = 1
 * 
 * Elementary test cases:
 *  There is one elementary test to perform for at least 1 bit rate
 *  configuration.
 * 
 *  Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces the last TQ of Phase_Seg2(D) of a recessive bit to dominant.
 *  The LT forces a following recessive bit to dominant from sync-segment up to
    Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D) − 1TQ(D).

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

class TestIso_8_8_4_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);

            assert(data_bit_timing.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
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
             *   2. Pick random recessive bit in data field which is followed by recessive bit.
             *   3. Force last TQ of picked bit to dominant.
             *   4. Force next bit from 2nd time quanta till one time quanta before sample point
             *      to dominant.
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

            random_bit->ForceTimeQuanta(data_bit_timing.ph2_ - 1, BitPhase::Ph2, BitValue::Dominant);

            // Note: ISO here says that this bit should be forced from SYNC. But that is clearly
            //       an error, because then there would not be two recessive to dominant edges!
            //       This should be reported to ISO! It should be forces from first bit of 
            for (size_t i = 1; i < data_bit_timing.prop_ + data_bit_timing.ph1_; i++)
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
        ENABLE_UNUSED_ARGS
};