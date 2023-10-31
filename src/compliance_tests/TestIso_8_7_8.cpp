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

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_7_8 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchType::Common);
            AddElemTestForEachSP(TestVariant::Common, true, FrameKind::Can20);

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElemTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* Generate test-specific bit timing with shifted sample point */
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

            uint8_t data_byte = 0x55;
            frm_flags = std::make_unique<FrameFlags>(FrameKind::Can20, RtrFlag::Data,
                                                       EsiFlag::ErrAct);
            gold_frm = std::make_unique<Frame>(*frm_flags, 0x1, &data_byte);
            RandomizeAndPrint(gold_frm.get());

            drv_bit_frm = ConvBitFrame(*gold_frm);
            mon_bit_frm = ConvBitFrame(*gold_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Shorten PH2 of second bit of data field by SJW in both driven and monitored
             *      frames. This corresponds to by how much the IUT shall resynchronize.
             *   3. Force all remaining time quantas of PH2 of this bit to 0 in driven frame.
             *      Together with step 2, this achieves shortening by whole PH2 of received frame,
             *      but following bit length is effectively lengthened for IUT, so that IUT will
             *      not have remaining phase error to synchronize away during next bits.
             *************************************************************************************/
            drv_bit_frm->GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

            drv_bit_frm->GetBitOf(1, BitKind::Data)
                ->ShortenPhase(BitPhase::Ph2, nbt.sjw_);

            // Due to input delay of DUT, the DUT will see the synchronization edge later, and thus
            // we need t expect that. Monitored frame must be shortened as resynchronization really
            // occurs! This does not corrupt the result of the test (next edge really occurs
            //  two bit times + PH2 - SJW ).
            Bit *mon_bit = mon_bit_frm->GetBitOf(1, BitKind::Data);
            mon_bit->ShortenPhase(BitPhase::Ph2, nbt.sjw_);
            if (mon_bit->GetPhaseLenCycles(BitPhase::Ph2) <= dut_input_delay) {
                auto tq = mon_bit->GetLastTQIter(BitPhase::Ph2);
                tq->Lengthen(
                    dut_input_delay - mon_bit->GetPhaseLenCycles(BitPhase::Ph2) + 1);
            }

            Bit *drv_bit = drv_bit_frm->GetBitOf(1, BitKind::Data);
            for (size_t i = 0; i < drv_bit->GetPhaseLenTQ(BitPhase::Ph2); i++)
                drv_bit->ForceTQ(i, BitPhase::Ph2, BitVal::Dominant);

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