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
            AddElemTestForEachSamplePoint(TestVariant::Common, true, FrameType::Can2_0);

            SetupMonitorTxTests();
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /* Generate test-specific bit timing with shifted sample point */
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

            uint8_t data_byte = 0x55;
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0, RtrFlag::DataFrame,
                                                       EsiFlag::ErrorActive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

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
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->GetBitOf(1, BitType::Data)
                ->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);

            // Due to input delay of DUT, the DUT will see the synchronization edge later, and thus
            // we need t expect that. Monitored frame must be shortened as resynchronization really
            // occurs! This does not corrupt the result of the test (next edge really occurs
            //  two bit times + PH2 - SJW ).
            Bit *mon_bit = monitor_bit_frm->GetBitOf(1, BitType::Data);
            mon_bit->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.sjw_);
            if (mon_bit->GetPhaseLenCycles(BitPhase::Ph2) <= dut_input_delay) {
                auto tq = mon_bit->GetLastTimeQuantaIterator(BitPhase::Ph2);
                tq->Lengthen(
                    dut_input_delay - mon_bit->GetPhaseLenCycles(BitPhase::Ph2) + 1);
            }

            Bit *drv_bit = driver_bit_frm->GetBitOf(1, BitType::Data);
            for (size_t i = 0; i < drv_bit->GetPhaseLenTimeQuanta(BitPhase::Ph2); i++)
                drv_bit->ForceTimeQuanta(i, BitPhase::Ph2, BitValue::Dominant);

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