/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.9.2
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point
 *        is the same as the bus value immediately after the edge on bit
 *        position DATA.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          DATA field
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the stuff bit to dominant from the second TQ(D)
 *             until the beginning of Phase_Seg2(D).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame containing a recessive stuff bit in data phase
 *  The LT forces a recessive stuff bit inside DATA field to dominant
 *  according to elementary test cases.
 *
 * Response:
 *  The modified stuff bit shall be sampled as dominant.
 *  The dominant sampled stuff bit shall be detected as a stuff error and
 *  shall be followed by an error frame..
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <cmath>

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

class TestIso_7_8_9_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1));
            
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::Shift);

            // Recessive stuff bit on 7-th data bit!
            uint8_t data_byte = 0x80;
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &data_byte);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force 7-th bit of data field to Dominant from 2nd time quanta till beginning
             *      of PH2.
             *   3. Insert expected active error frame on monitored frame from 8-th bit of data
             *      field. Insert passive error frame on driven frame!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *stuff_bit = driver_bit_frm->GetBitOf(6, BitType::Data);
            stuff_bit->ForceTimeQuanta(1, data_bit_timing.prop_ + data_bit_timing.ph1_,
                                       BitValue::Dominant);

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertActiveErrorFrame(7, BitType::Data);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("DontShift synchronisation after dominant bit sampled on Data field bit!");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};