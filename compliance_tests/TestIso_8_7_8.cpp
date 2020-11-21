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
 * @test ISO16845 8.7.8
 * 
 * @brief The purpose of this test is to verify that an IUT transmitting will
 *        synchronize correctly in case of a resynchronization as a result of
 *        a recessive to dominant edge that occurs immediately after the
 *        sample point.
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
 *  There is one test for each programmable sampling point inside a chosen
 *  number of TQ for at least 1 bit rate configuration.
 *      #1 The LT shortens the recessive bit by an amount of Phase_Seg2(N).
 *  
 *  Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame that contains a
 *  sequence of 4 alternating recessive and dominant bits.
 *  While the IUT is transmitting the frame, the LT shortens the first
 *  recessive bit of the alternating sequence according to elementary test
 *  cases and sends the next dominant bit.
 * 
 * Response:
 *  The next edge from recessive to dominant sent by the IUT shall occur two
 *  CAN bit times + [Phase_Seg2(N) – SJW(N)] after the edge applied by the LT
 *  and the IUT shall continue transmitting the frame.
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

class TestIso_8_7_8 : public test_lib::TestBase
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
            uint8_t data_byte = 0x55;

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                for (auto elem_test : elem_tests[test_variant])
                {
                    PrintElemTestInfo(elem_test);

                    frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0,
                                        RtrFlag::DataFrame, EsiFlag::ErrorActive);
                    golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
                    RandomizeAndPrint(golden_frm.get());

                    driver_bit_frm = ConvertBitFrame(*golden_frm);
                    monitor_bit_frm = ConvertBitFrame(*golden_frm);

                    /******************************************************************************
                     * Modify test frames:
                     *   1. Insert ACK to driven frame.
                     *   2. Shorten PH2 of second bit of data field by SJW in both driven and
                     *      monitored frames. This corresponds to by how much the IUT shall
                     *      resynchronize.
                     *   3. Force all remaining time quantas of PH2 of this bit to 0 in driven
                     *      frame. Together with step 2, this achvies shortening by whole PH2
                     *      of received frame, but following bit length is effectively lengthened
                     *      for IUT, so that IUT will not have remaining phase error to synchronize
                     *      away during next bits.
                     *****************************************************************************/
                    driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                    driver_bit_frm->GetBitOf(1, BitType::Data)
                        ->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);
                    monitor_bit_frm->GetBitOf(1, BitType::Data)
                        ->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);

                    Bit *drv_bit = driver_bit_frm->GetBitOf(1, BitType::Data);
                    for (size_t i = 0; i < drv_bit->GetPhaseLenTimeQuanta(BitPhase::Ph2); i++)
                        drv_bit->ForceTimeQuanta(i, BitPhase::Ph2, BitValue::Dominant);

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