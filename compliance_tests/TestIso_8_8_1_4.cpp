/***************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
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

class TestIso_8_8_1_4 : public test_lib::TestBase
{
    public:
        BitTiming test_nom_bit_timing;
        BitTiming test_data_bit_timing;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);

            // Elementary test for each possible positon of sample point between: (2, NTQ-1)
            // Note that this test verifies BRS bit, so we need to alternate also data bit timing!
            // This will then affect overall bit-rate!
            for (size_t i = 0; i < data_bit_timing.GetBitLengthTimeQuanta() - 2; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::Can2_0));

            dut_ifc->ConfigureSsp(SspType::Disabled, 0);

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);

            assert(data_bit_timing.brp_ > 2 &&
                   "TQ(D) shall bigger than 2 for this test due to test architecture!");
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            // Calculate new bit-rate from configured one. Modify PROP + PH2 of data bit rate.
            // Modify only PH2 of nominal bit-rate. This should be sufficient for various sample
            // point positions within CRC delimiter.
            test_nom_bit_timing.brp_ = nominal_bit_timing.brp_;
            test_nom_bit_timing.sjw_ = nominal_bit_timing.sjw_;
            test_nom_bit_timing.ph1_ = 0;
            test_nom_bit_timing.prop_ = nominal_bit_timing.prop_;
            test_nom_bit_timing.ph2_ = data_bit_timing.GetBitLengthTimeQuanta() - elem_test.index - 1;
            
            test_data_bit_timing.brp_ = data_bit_timing.brp_;
            test_data_bit_timing.sjw_ = data_bit_timing.sjw_;
            test_data_bit_timing.ph1_ = 0;
            test_data_bit_timing.prop_ = elem_test.index;
            test_data_bit_timing.ph2_ = data_bit_timing.GetBitLengthTimeQuanta() - elem_test.index - 1;

            /* Re-configure bit-timing for this test so that frames are generated with it! */
            this->nominal_bit_timing = test_nom_bit_timing;
            this->data_bit_timing = test_data_bit_timing;

            // Reconfigure DUT with new Bit time config with same bit-rate but other SP.
            dut_ifc->Disable();
            dut_ifc->ConfigureBitTiming(test_nom_bit_timing, test_data_bit_timing);
            dut_ifc->Enable();
            TestMessage("Waiting till DUT is error active!");
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(100000);

            TestMessage("Nominal bit timing for this elementary test:");
            test_nom_bit_timing.Print();
            TestMessage("Data bit timing for this elementary test:");
            test_data_bit_timing.Print();


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
            for (size_t i = 0; i < data_bit_timing.brp_; i++)
                first_ph2_tq->ForceCycleValue(i, BitValue::Recessive);

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
        ENABLE_UNUSED_ARGS
};