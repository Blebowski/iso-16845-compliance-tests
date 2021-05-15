/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 15.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.7.2
 * 
 * @brief The purpose of this test is to verify that the IUT, with a pending
 *        transmission, makes a hard synchronization when detecting a dominant
 *        bit after the sample point of the third bit of the intermission field.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 * 
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      ID = MSB dominant
 *      FDF = 0
 * 
 * Elementary test cases:
 *   Test each possible sampling point inside a chosen number of TQ for at
 *   least 1 bit rate configuration.
 * 
 *      #1 LT generates a dominant bit starting IPT after the sample point.  
 * 
 *   Refer to 6.2.3.
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame according to
 *  elementary test cases.
 *  The LT disturbs the transmission of the IUT to cause a re-transmission.
 *  While the IUT’s transmission is pending, the LT generates a dominant
 *  bit according to elementary test cases after the sample point of the
 *  third bit of the intermission field.
 *  
 * Response:
 *  The IUT shall start transmitting its SOF at the next TQ following the
 *  recessive to dominant edge.
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

class TestIso_8_7_3 : public test_lib::TestBase
{
    public:
        BitTiming test_nom_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTestForEachSamplePoint(TestVariant::Common, true, FrameType::Can2_0);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, backup_nominal_bit_timing);
            ReconfigureDutBitTiming();
            WaitDutErrorActive();

            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base,
                            RtrFlag::DataFrame, BrsFlag::DontShift, EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x7FF, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Corrupt 7-th bit of data field (should be recessive stuff bit) in driven frame.
             *   2. Insert active error frame from next bit on to monitored frame. Insert passive
             *      error frame to driven frame.
             *   3. Shorten third bit of Intermission by Phase 2 length - IPT. Do this in both,
             *      driven and monitored frames!
             *   4. Lengthen SYNC of the same bit by 1 TQ in monitored frame. This will offset
             *      SOF transmitted by next frame by 1 TQ!
             *   5. In second monitored frame, turn first Time quanta recessive. This is the time
             *      quanta during which driven frame already has dominant value transmitted!
             *   6. Append retransmitted frame as if received. Reduce SOF lenght in monitored frame
             *      by 1 TQ since in last bit of intermission, it was prolonged.
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            Bit *last_interm_bit_drv = driver_bit_frm->GetBitOf(2, BitType::Intermission);
            Bit *last_interm_bit_mon = monitor_bit_frm->GetBitOf(2, BitType::Intermission);

            last_interm_bit_drv->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_ - 1);
            last_interm_bit_mon->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_ - 1);

            last_interm_bit_drv->GetTimeQuanta(BitPhase::Ph2, 0)
                ->Shorten(nominal_bit_timing.brp_ - dut_ipt);
            last_interm_bit_mon->GetTimeQuanta(BitPhase::Ph2, 0)
                ->Shorten(nominal_bit_timing.brp_ - dut_ipt);

            /* This trick needs to be done to check that IUT transmits the first TQ recessive.
             * During this TQ, LT still sends the hard sync edge. This corresponds to
             * description in "response". Using "force" functions on first time quanta of
             * seconds frame SOF cant be used since "force" is only used on driven bits!
             */
            last_interm_bit_mon->LengthenPhase(BitPhase::Sync, 1);
            monitor_bit_frm_2->GetBitOf(0, BitType::Sof)->ShortenPhase(BitPhase::Sync, 1);

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm_2->GetBitOf(0, BitType::Sof)->bit_value_ = BitValue::Dominant;
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

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