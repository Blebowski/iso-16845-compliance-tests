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
 * @date 28.12.2020
 *
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.8.1.4
 *
 * @brief The purpose of this test is to verify the sample point of an IUT
 *        acting as a transmitter on bit position CRC delimiter.
 * @version CAN FD enabled
 *
 * Test variables:
 *      Sampling_Point(D) configuration as available by IUT.
 *          CRC delimiter
 *          BRS = 1
 *          FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform for each programmable sampling point
 *  inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 Check sampling point by applying the correct bit value only at
 *         programmed position of sampling point by
 *          [Sync_Seg(D) + Prop_Seg(D) + Phase_Seg1(D)].
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation is disabled.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame with a recessive bit value at
 *  last bit of CRC.
 *  The LT forces the CRC delimiter to dominant and insert a recessive pulse
 *  of 2 TQ(D) around the sampling point according to elementary test cases.
 *
 * Response:
 *  The modified CRC delimiter bit shall be sampled as recessive.
 *  The frame is valid. No error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_1_4 : public test::TestBase
{
    public:
        BitTiming test_nom_bit_timing;
        BitTiming test_data_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            AddElemTestForEachSamplePoint(TestVariant::CanFdEnabled, false, FrameType::Can2_0);

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);

            SetupMonitorTxTests();

            assert(data_bit_timing.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            // Generate new test-specific bit timings
            // Keep both bit-rates the same to make the sample point generation simple!
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, true);
            data_bit_timing = GenerateSamplePointForTest(elem_test, false);

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(nominal_bit_timing, data_bit_timing);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            nominal_bit_timing.Print();
            TestMessage("Data bit timing for this elementary test:");
            data_bit_timing.Print();


            uint8_t data = 0x55;
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, IdentifierType::Base,
                                    RtrFlag::DataFrame, BrsFlag::Shift, EsiFlag::ErrorActive);
            /* Put exact frame so that we are sure that last bit of CRC is recessive */
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0xAA, &data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Force CRC Delimiter to Dominant.
             *   3. Force last TQ of phase before PH2 to recessive. Force fircst BRP(DBT) of PH2
             *      to recessive.
             *************************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *crc_delim = driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter);
            crc_delim->bit_value_ = BitValue::Dominant;

            BitPhase prev_phase = crc_delim->PrevBitPhase(BitPhase::Ph2);
            auto it = crc_delim->GetLastTimeQuantaIterator(prev_phase);
            it->ForceValue(BitValue::Recessive);

            TimeQuanta *first_ph2_tq = crc_delim->GetTimeQuanta(BitPhase::Ph2, 0);

            for (size_t i = 0; i < nominal_bit_timing.brp_; i++){
                first_ph2_tq->ForceCycleValue(i, BitValue::Recessive);
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

            FreeTestObjects();
            return FinishElementaryTest();
        }
};