/*****************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 15.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.7.3
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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_3 : public test::TestBase
{
    public:
        BitTiming test_nom_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            AddElemTestForEachSamplePoint(TestVariant::Common, true, FrameKind::Can20);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            assert(dut_input_delay == dut_ipt && "Needed due to test assumptions!");
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, true);
            ReconfigureDutBitTiming();
            WaitDutErrorActive();

            uint8_t data_byte = 0x80;
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentKind::Base,
                            RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
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
            driver_bit_frm->ConvRXFrame();
            driver_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            driver_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            monitor_bit_frm->InsertActErrFrm(7, BitKind::Data);

            Bit *last_interm_bit_drv = driver_bit_frm->GetBitOf(2, BitKind::Interm);
            Bit *last_interm_bit_mon = monitor_bit_frm->GetBitOf(2, BitKind::Interm);

            // Remove whole PH2
            while (last_interm_bit_drv->GetPhaseLenTQ(BitPhase::Ph2) > 0)
            {
                last_interm_bit_drv->ShortenPhase(BitPhase::Ph2, 1);
                last_interm_bit_mon->ShortenPhase(BitPhase::Ph2, 1);
            }

            // Here we lenghten only monitored bit! Since Input delay of CTU CAN FD is the same
            // as its IPT, if we added IPT also on driven frame, we would need to compensate
            // for the same value. Since input delay is the same as IPT, if we add nothing, the
            // edge will anyway arrive IPT after sample point! By adding IPT to monitored frame,
            // we acccount for IPT in IUTs perception of the synchronization edge on RX!
            // TODO: This works only for controllers which have IPT = Input Delay!
            last_interm_bit_mon->GetTQ(last_interm_bit_mon->GetLenTQ() - 1)
                ->Lengthen(dut_ipt);

            /* This trick needs to be done to check that IUT transmits the first TQ recessive.
             * During this TQ, LT still sends the hard sync edge. This corresponds to
             * description in "response". Using "force" functions on first time quanta of
             * seconds frame SOF cant be used since "force" is only used on driven bits!
             */
            last_interm_bit_mon->LengthenPhase(BitPhase::Sync, 1);
            monitor_bit_frm_2->GetBitOf(0, BitKind::Sof)->ShortenPhase(BitPhase::Sync, 1);

            driver_bit_frm_2->ConvRXFrame();
            driver_bit_frm_2->GetBitOf(0, BitKind::Sof)->val_ = BitVal::Dominant;
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