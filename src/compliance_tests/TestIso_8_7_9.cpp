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
 * @date 21.11.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.7.9
 *
 * @brief The purpose of this test is to verify the behaviour of an IUT, acting
 *        as a transmitter, synchronizing to a recessive to dominant edge after
 *        the sample point, while sending a dominant bit. The edge is caused by
 *        a disturbance of the dominant bit.
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
 *  There is one elementary test to perform for each programmable sampling point
 *  inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 LT sends two time quanta recessive state, starting one time quanta
 *         before the sample point of the IUT.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a Classical CAN frame.
 *  While the IUT sends a dominant bit, the LT sends two time quanta recessive
 *  state, according to elementary test cases.
 *
 * Response:
 *  The IUT sends an error flag and the next edge sent by the IUT occurs 6
 *  bit times + [Phase_Seg2(N) – SJW(N)] after the recessive to dominant edge
 *  applied by the LT after the sample point of the dominant bit
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_9 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTestForEachSP(TestVariant::Common, true, FrameKind::Can20);

            SetupMonitorTxTests();

            assert(nbt.brp_ > 2 &&
                   "TQ(N) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Generate test specific bit timing
            nbt = GenerateSPForTest(elem_test, true);

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(nbt, dbt);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfState::ErrAct)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            nbt.Print();

            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            drv_bit_frm_2 = ConvBitFrame(*gold_frm);
            mon_bit_frm_2 = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Choose random dominant bit from driven frame.
             *   2. Force last time Quanta of phase before PH2 and first time quanta of
             *      Phase 2 to recessive.
             *   3. Compensate position of inserted two time quanta glitch. Shift it by IUT
             *      input delay, so that IUTs perception of the glitch is exactly the two time
             *      quanta around the sample point.
             *   4. Shorten PH2 of driven and monitored frames by SJW. This correponds to
             *      by how much IUT should have resynchronized.
             *   5. Insert Active Error frame from next bit on.
             *   6. Append retransmitted frame
             *************************************************************************************/
            Bit *rand_bit = drv_bit_frm->GetRandBit(BitVal::Dominant);
            int rand_bit_index = drv_bit_frm->GetBitIndex(rand_bit);

            rand_bit->ForceTQ(0, BitPhase::Ph2, BitVal::Recessive);
            rand_bit->GetLastTQIter(rand_bit->PrevBitPhase(BitPhase::Ph2))
                ->ForceVal(BitVal::Recessive);

            // TODO: Execute compensation of position of inserted two time quanta glitch. We should
            //       take into account IUTs input delay and shift the glitch by dut_input_delay
            //       time quantas sho that IUT receives the glitch exactly the two time quanta
            //       around the sample point from its perception!
            //       If we do this, we no more need min TQ(N) == 2!

            rand_bit->ShortenPhase(BitPhase::Ph2, nbt.sjw_);
            mon_bit_frm->GetBit(rand_bit_index)->ShortenPhase(BitPhase::Ph2,
                nbt.sjw_);

            drv_bit_frm->InsertActErrFrm(rand_bit_index + 1);
            mon_bit_frm->InsertActErrFrm(rand_bit_index + 1);

            drv_bit_frm_2->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
            drv_bit_frm->AppendBitFrame(drv_bit_frm_2.get());
            mon_bit_frm->AppendBitFrame(mon_bit_frm_2.get());

            drv_bit_frm->Print(true);
            mon_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLT(*drv_bit_frm, *mon_bit_frm);
            StartDrvAndMon();
            dut_ifc->SendFrame(gold_frm.get());
            WaitForDrvAndMon();
            CheckLTResult();

            return FinishElemTest();
        }

};