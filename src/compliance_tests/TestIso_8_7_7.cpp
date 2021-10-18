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
 * @test ISO16845 8.7.7
 * 
 * @brief The purpose of this test is to verify that an IUT transmitting a
 *        dominant bit does not perform any resynchronization as a result of a
 *        recessive to dominant edge with a positive phase error.
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
 *      #1 The LT delays each recessive to dominant edge by 2 time quanta.
 *  
 *  Refer to 6.2.3
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  While the IUT is transmitting the frame, the LT delays each recessive to
 *  dominant according to elementary test cases.
 * 
 * Response:
 *  The IUT shall continue transmitting frame without any resynchronization.
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

class TestIso_8_7_7 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTestForEachSamplePoint(TestVariant::Common, true, FrameType::Can2_0);

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Minimal lenght of PH1/PROP must be such that the delayed edge by two time
            // quanta + input delay of IUT will still guarantee that proper value of
            // transmitted bit will propagate to IUTs reception in time!
            // For CTU CAN FD, this is:
            //      input delay (2) + delay of edge due to test (2) + 1 = 5. Minimal
            //      duration of TSEG1 = 5 cycles!!!
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, true, 4);

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(nominal_bit_timing, data_bit_timing);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            nominal_bit_timing.Print();

            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Search through CAN frame and search for each dominant bit following recessive
             *      bit. For each such bit, force its first two time quantas to recessive. This
             *      delays the synchronization edge by two time quantas.
             * 
             * Note: TX/RX feedback must be disabled, since we modify driven frame.
             * Note: The overall lenght of each bit is kept! Therefore, if IUT will  synchronize,
             *       then it will be off from monitored frame whose bits are not prolonged/shorte-
             *       ned in any way! So monitoring frame succesfully, checks that no synchroniza-
             *       tion has been done! This behavior has been checked with bug inserted into IUT
             *       and the test really failed!
             *************************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            // I know this search is performance suicide, but again, we dont give a shit
            // about performance here... It is just a model running on powerfull PC!
            for (size_t i = 0; i < driver_bit_frm->GetBitCount() - 1; i++)
            {
                Bit *curr = driver_bit_frm->GetBit(i);
                Bit *next = driver_bit_frm->GetBit(i + 1);

                if (curr->bit_value_ == BitValue::Recessive &&
                    next->bit_value_ == BitValue::Dominant)
                {
                    next->GetTimeQuanta(0)->ForceValue(BitValue::Recessive);
                    next->GetTimeQuanta(1)->ForceValue(BitValue::Recessive);
                }
            }

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