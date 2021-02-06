/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 22.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.23
 * 
 * @brief The purpose of this test is to verify that the IUT switches to
 *        protocol exception on non-nominal values of the bits described in
 *        test variables and did not change the CAN error counter.
 * @version CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  CAN FD Tolerant:
 *      FDF = 1
 *      DLC
 *      Data: All data byte with the same value
 *      Bit rate ratio between nominal and data bit rate
 * 
 *  CAN FD enabled:
 *      FDF = 1
 *      “res” bit = 1
 *      DLC
 *      Data: All data byte with the same value
 *      Bit rate ratio between nominal and data bit rate
 * 
 * Elementary test cases:
 *   CAN FD Tolerant:
 *      Test    Format      DLC         Data        Bit rate ratio   
 *       #1      FBFF       0xA         0xAA            1:2
 *       #2      FBFF       0xF         0xFF            1:8
 *       #3      CBFF       0xF         0xFF             -
 * 
 *   CAN FD Enabled:
 *       #1      FBFF       0xA         0xAA            1:2
 *       #2      FBFF       0xF         0xFF            1:8
 *       #3      CBFF       0xF         0xFF             -  
 * 
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  a) The test system causes a receive error to initialize the REC value to 9.
 *  b) A single test frame is used for the elementary test, followed immediately
 *     by a valid Classical CAN frame.
 * 
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall not acknowledge the test frame.
 *  A following data frame in classical frame format received by the IUT during
 *  the test state shall match the data sent in the test frame.
 *  The IUT’s REC value shall be 8 after reception of the valid Classical
 *  CAN frame.
 * 
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
#include "../test_lib/ElementaryTest.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;
using namespace test_lib;

class TestIso_7_6_23 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::FdTolerantFdEnabled);

            int num_elem_tests;
            if (test_variants[0] == TestVariant::CanFdTolerant)
                num_elem_tests = 3;
            else if (test_variants[0] == TestVariant::CanFdEnabled)
                num_elem_tests = 2;
            else
                num_elem_tests = 0;

            for (int i = 0; i < num_elem_tests; i++)
                if (i < 3)
                    elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::CanFd));
                else
                    elem_tests[0].push_back(ElementaryTest(i + 1, FrameType::Can2_0));

            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            /******************************************************************************
             * First configure bit rate. Take configured bit rate for CAN FD and multiply
             * by 8/4 to get data bit rate. This-way we hope not to get out of DUTs spec
             * for bit timing!
             *****************************************************************************/
            dut_ifc->Disable();
            dut_ifc->ConfigureProtocolException(true);
            nominal_bit_timing = data_bit_timing;
            if (elem_test.index == 1)
                nominal_bit_timing.brp_ = data_bit_timing.brp_ * 2;
            else
                nominal_bit_timing.brp_ = data_bit_timing.brp_ * 8;
            dut_ifc->ConfigureBitTiming(nominal_bit_timing, data_bit_timing);
            
            /* Enable and wait till integration is over again */
            dut_ifc->Enable();
            while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
                usleep(2000);

            /******************************************************************************
             * Generate frames!
             *****************************************************************************/
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type);

            if (elem_test.index == 1)
            {
                uint8_t data[64] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
                golden_frm = std::make_unique<Frame>(*frame_flags, 0xA, data);
            } else {
                uint8_t data[64] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                golden_frm = std::make_unique<Frame>(*frame_flags, 0xF, data);
            }
            RandomizeAndPrint(golden_frm.get());

            frame_flags_2 = std::make_unique<FrameFlags>(FrameType::Can2_0);
            golden_frm_2 = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**********************************************************************************
             * Modify test frames:
             *   1. Modify test frame according to elementary test cases. FD Tolerant variant
             *      needs no modifications since FDF recessive is enough to trigger protocol
             *      exception! FD Enabled needs bit after FDF forced recessive!
             *   2. Update the frames since this might have changed CRC/lenght.
             *   3. Turn monitored frame as if received!
             *   4. Remove ACK from monitored frame (since IUT is in protocol exception). No
             *      other modifications are needed since if monitored frame is as if received,
             *      IUT transmitts all recessive! IUT should be now monitoring until it
             *      receives 11 consecutive recessive bits!
             *   5. Append second frame directly after first frame as if transmitted by LT.
             **********************************************************************************/
            if (test_variant == TestVariant::CanFdEnabled)
            {
                driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
            }
            
            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            monitor_bit_frm->TurnReceivedFrame();

            monitor_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Recessive;

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2->TurnReceivedFrame();

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            /********************************************************************************** 
             * Execute test
             *********************************************************************************/
            dut_ifc->SetRec(9);
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm_2);
            CheckRecChange(rec_old, -1);

            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};