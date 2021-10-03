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
 *        bit before the sample point of the third bit of the intermission
 *        field.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN
 *  CAN FD tolerant
 *  CAN FD enabled
 * 
 *  Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      ID = MSB recessive
 *      FDF = 0
 * 
 * Elementary test cases:
 *   Test each possible sampling point inside a chosen number of TQ for at least
 *   1 bit rate configuration.
 *      #1 Dominant bit starting [1 TQ(N) + minimum time quantum] before the
 *         sample point of the third bit of the intermission field.
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
 *  While the IUT’s transmission is pending, the LT generates a dominant bit
 *  according to elementary test cases before the sample point of the third
 *  bit of the intermission field.
 *  
 * Response:
 *  The IUT shall start transmitting the first bit of the identifier 1 bit
 *  time −1 TQ or up to 1 bit time after the recessive to dominant edge of
 *  the SOF.
 *  The first edge inside the arbitration field has to be transmitted an
 *  integer number of bit time −1 TQ or up to an integer number of bit time
 *  after the SOF edge.
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

class TestIso_8_7_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTestForEachSamplePoint(TestVariant::Common, true, FrameType::Can2_0);
            
            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            assert((nominal_bit_timing.brp_ > 1 &&
                    "BRP Nominal must be bigger than 1 in this test due to test architecture!"));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, true);
            ReconfigureDutBitTiming();
            WaitDutErrorActive();

            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base,
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
             *   3. Shorten third bit of Intermission by Phase 2 length and shorten Phase 1 segment
             *      by 1 TQ + 1 minimal time quanta (1 cycle). Do this in both driven and monitored
             *      frames.
             *   4. Turn Second driven frame received, but keep SOF dominant.
             *   5. Turn SOF of second monitored frame to recessive (since it will not be trans-
             *      mitted). Shorten that SOF by 1 TQ (since Hard sync ends with one SYNC segment
             *      completed).
             *   6. Append second frame after the first frame.
             *   7. Compensate edge position from Intermission to Second SOF for input delay of
             *      IUT.
             *   
             *   Note: First frame ends 1 TQ - 1 minimal TQ before the end of Intermission bit 2
             *         (in both driven and monitored frames). After this, monitored frame is appen-
             *         ded, just with SOF dominant. This means that dominant bit will be received
             *         by IUT as required! IUT will h-sync, but it is still in intermission. So
             *         first bit of second frame will be transmitted recessive. This is emulated
             *         by SOF of second monitored frame!
             *************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            Bit *last_interm_bit_drv = driver_bit_frm->GetBitOf(2, BitType::Intermission);
            Bit *last_interm_bit_mon = monitor_bit_frm->GetBitOf(2, BitType::Intermission);

            last_interm_bit_drv->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            BitPhase prev_phase_drv = last_interm_bit_drv->PrevBitPhase(BitPhase::Ph2);
            last_interm_bit_drv->ShortenPhase(prev_phase_drv, 1);
            last_interm_bit_drv->GetLastTimeQuantaIterator(prev_phase_drv)->Shorten(1);

            last_interm_bit_mon->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);                    
            BitPhase prev_phase_mon = last_interm_bit_mon->PrevBitPhase(BitPhase::Ph2);
            last_interm_bit_mon->ShortenPhase(prev_phase_mon, 1);
            last_interm_bit_mon->GetLastTimeQuantaIterator(prev_phase_mon)->Shorten(1);

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm_2->GetBitOf(0, BitType::Sof)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm_2->GetBitOf(0, BitType::Sof)->ShortenPhase(BitPhase::Sync, 1);
            monitor_bit_frm_2->GetBitOf(0, BitType::Sof)->bit_value_ = BitValue::Recessive;

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->CompensateEdgeForInputDelay(
                driver_bit_frm->GetBitOf(1, BitType::Sof), this->dut_input_delay);

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