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
 * @test ISO16845 7.8.9.1
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        BRS.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(N) and SJW(N) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          BRS = 0
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the first two TQ(N) and the complete Phase_Seg2(N)
 *             of BRS bit to recessive.
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT sends a frame with dominant BRS bit.
 *  The LT inverts parts of BRS bit according to elementary test cases.
 *
 * Response:
 *  The modified BRS bit shall be sampled as dominant.
 *  The frame is valid. No error flag shall occur. The bit rate will not
 *  switch for data phase.
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

class TestIso_7_8_9_1 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1));
            
            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            // Here we have to set Bit rate dont shift because we intend to get BRS dominant,
            // so bit rate should not be shifted!
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, BrsFlag::DontShift);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Flip BRS value to dominant.
             *   3. Force first two TQ of BRS and Phase 2 of BRS to Recessive. This should cause
             *      resynchronisation edge with phase error 2, but DUT shall ignore it and not
             *      resynchronize because previous bit (r0) was Dominant!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *brs_bit = driver_bit_frm->GetBitOf(0, BitType::Brs);
            
            brs_bit->bit_value_ = BitValue::Dominant;
            
            brs_bit->ForceTimeQuanta(0, BitValue::Recessive);
            brs_bit->ForceTimeQuanta(1, BitValue::Recessive);

            // Force all TQ of PH2 as if no shift occured (this is what frame was generated with)
            brs_bit->ForceTimeQuanta(0, nominal_bit_timing.ph2_ - 1,
                                     BitPhase::Ph2, BitValue::Recessive);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("No synchronisation after dominant bit sampled on BRS bit!");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);

            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};