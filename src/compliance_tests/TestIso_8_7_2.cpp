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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_2 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTestForEachSP(TestVariant::Common, true, FrameKind::Can20);

            SetupMonitorTxTests();
            CanAgentConfigureTxToRxFeedback(true);

            TEST_ASSERT(nbt.brp_ > 1,
                        "BRP Nominal must be bigger than 1 in this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            nbt = GenerateSPForTest(elem_test, true);
            ReconfDutBitTiming();
            WaitDutErrAct();

            uint8_t data_byte = 0x80;
            frm_flags = std::make_unique<FrameFlags>(elem_test.frame_kind_, IdentKind::Base,
                                RtrFlag::Data, BrsFlag::NoShift, EsiFlag::ErrPas);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, 0x7FF, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

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
            drv_bit_frm->ConvRXFrame();
            drv_bit_frm->GetBitOf(6, BitKind::Data)->FlipVal();

            drv_bit_frm->InsertPasErrFrm(7, BitKind::Data);
            mon_bit_frm->InsertActErrFrm(7, BitKind::Data);

            Bit *last_interm_bit_drv = drv_bit_frm->GetBitOf(2, BitKind::Interm);
            Bit *last_interm_bit_mon = mon_bit_frm->GetBitOf(2, BitKind::Interm);

            last_interm_bit_drv->ShortenPhase(BitPhase::Ph2, nbt.ph2_);
            BitPhase prev_phase_drv = last_interm_bit_drv->PrevBitPhase(BitPhase::Ph2);
            last_interm_bit_drv->ShortenPhase(prev_phase_drv, 1);
            last_interm_bit_drv->GetLastTQIter(prev_phase_drv)->Shorten(1);

            last_interm_bit_mon->ShortenPhase(BitPhase::Ph2, nbt.ph2_);
            BitPhase prev_phase_mon = last_interm_bit_mon->PrevBitPhase(BitPhase::Ph2);
            last_interm_bit_mon->ShortenPhase(prev_phase_mon, 1);
            last_interm_bit_mon->GetLastTQIter(prev_phase_mon)->Shorten(1);

            drv_bit_frm_2->ConvRXFrame();
            drv_bit_frm_2->GetBitOf(0, BitKind::Sof)->val_ = BitVal::Dominant;

            mon_bit_frm_2->GetBitOf(0, BitKind::Sof)->ShortenPhase(BitPhase::Sync, 1);
            mon_bit_frm_2->GetBitOf(0, BitKind::Sof)->val_ = BitVal::Recessive;

            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->CompensateEdgeForInputDelay(
                drv_bit_frm->GetBitOf(1, BitKind::Sof), this->dut_input_delay);

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             *****************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};