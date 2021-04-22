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
 * @test ISO16845 8.7.6
 * 
 * @brief The purpose of this test is to verify that there is only one
 *        synchronization within 1 bit time if there are two recessive to
 *        dominant edges between two sample points where the first edge comes
 *        before the synchronization segment.
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
 *  There is one elementary test to perform for each programmable sampling
 *  point inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 Recessive glitch at 2nd TQ in early started dominant bit.
 *
 *  Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  A recessive bit which is followed by a dominant bit is shortened by one
 *  time quantum.
 *  After one time quantum of dominant value, the LT forces one time quantum
 *  to recessive value.
 * 
 * Response:
 *  The IUT shall send a dominant to recessive edge an integer number of bit
 *  times after the first recessive to dominant edge applied by the LT.
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

class TestIso_8_7_6 : public test_lib::TestBase
{
    public:
        BitTiming test_nom_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);

            // Elementary test for each possible positon of sample point, restrict to shortest
            // possible PROP = 2, shortest possible PH2 = 1, PH1 always 0. Together we test
            // TQ(N) - 3 tests!
            for (size_t i = 1; i < nominal_bit_timing.GetBitLengthTimeQuanta() - 2; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetWaitForMonitor(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Calculate new bit-rate from configured one. Have same bit-rate, but
            // different sample point. Shift sample point from 2 TQ up to 1 TQ before the
            // end.
            test_nom_bit_timing.brp_ = nominal_bit_timing.brp_;
            test_nom_bit_timing.sjw_ = nominal_bit_timing.sjw_;
            test_nom_bit_timing.ph1_ = 0;
            test_nom_bit_timing.prop_ = elem_test.index;
            test_nom_bit_timing.ph2_ = nominal_bit_timing.GetBitLengthTimeQuanta() - elem_test.index - 1;
            
            /* Re-configure bit-timing for this test so that frames are generated with it! */
            this->nominal_bit_timing = test_nom_bit_timing;

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(test_nom_bit_timing, data_bit_timing);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            test_nom_bit_timing.Print();

            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /******************************************************************************
             * Modify test frames:
             *   1. Set Ack dominant in driven frame.
             *   2. Choose random recessive bit which is followed by dominant bit.
             *   3. Shorten chosen bit by 1 TQ in driven and monitored frames. This
             *      corresponds to resynchronization by 1 TQ.
             *   4. Force 2nd Time quanta of dominant bit after the random recessive bit
             *      to Recessive in driven frame.
             * 
             *   Note: The check that next edge shall be sent integer number of time
             *         quantas after it is executed by the fact that we don not do any
             *         further resynchronizations. Model takes care of it then!
             *****************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *rand_bit = nullptr;
            Bit *next_bit = nullptr;
            size_t rand_bit_index;
            do {
                rand_bit = driver_bit_frm->GetRandomBit(BitValue::Recessive);
                rand_bit_index = driver_bit_frm->GetBitIndex(rand_bit);
                if (rand_bit_index == driver_bit_frm->GetBitCount() - 1)
                    continue;
                next_bit = driver_bit_frm->GetBit(rand_bit_index + 1);
            } while (next_bit == nullptr || next_bit->bit_value_ == BitValue::Recessive);

            rand_bit->ShortenPhase(BitPhase::Ph2, 1);
            monitor_bit_frm->GetBit(rand_bit_index)->ShortenPhase(BitPhase::Ph2, 1);

            next_bit->ForceTimeQuanta(1, BitValue::Recessive);

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

            return FinishElementaryTest();
        }

};