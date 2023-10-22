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
 * @test ISO16845 8.8.1.1
 *
 * @brief The purpose of this test is to verify the sample point of an IUT
 *        acting as a transmitter on bit position “res” bit.
 * @version CAN FD enabled
 *
 * Test variables:
 *  CAN FD enabled
 *
 *  Sampling_Point(N) configuration as available by IUT.
 *      “res” bit
 *      BRS = 1
 *      FDF = 1
 *
 * Elementary test cases:
 *  There is one elementary test to perform for each programmable sampling
 *  point inside a chosen number of TQ for at least 1 bit rate configuration.
 *      #1 “res” bit level changed to recessive after sampling point.
 *
 *  Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  Transmitter delay compensation is disabled.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame.
 *  The LT forces Phase_Seg2(N) of “res” bit to recessive according to
 *  elementary test cases.
 *
 * Response:
 *  The modified “res” bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur.
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "TestBase.h"

using namespace can;
using namespace test;

class TestIso_8_8_1_1 : public test::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);

            AddElemTestForEachSamplePoint(TestVariant::CanFdEnabled, true, FrameType::Can2_0);

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);
            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {

            /* Re-configure bit-timing for this test so that frames are generated with it! */
            nominal_bit_timing = GenerateSamplePointForTest(elem_test, true);

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(nominal_bit_timing, data_bit_timing);
            dut_ifc->Enable();

            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            nominal_bit_timing.Print();


            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Insert ACK to driven frame.
             *   2. Force whole Phase 2 segment of "res" (r0) bit to dominant in driven frame.
             *************************************************************************************/
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            Bit *res_bit = driver_bit_frm->GetBitOf(0, BitType::R0);
            for (size_t i = 0; i < res_bit->GetPhaseLenTimeQuanta(BitPhase::Ph2); i++)
                res_bit->ForceTimeQuanta(i, BitPhase::Ph2, BitValue::Recessive);

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