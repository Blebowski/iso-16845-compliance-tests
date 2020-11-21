/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 3.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.4
 * 
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid base
 *        format frame.
 * 
 * @version CAN FD Enabled, Classical CAN
 * 
 * Test variables:
 *  Classical CAN  : FDF = 1
 *  CAN FD Enabled : FDF = 1, RRS = 1
 * 
 * Elementary test cases:
 *  Classical CAN:
 *      #1 FDF = 1
 * 
 *  CAN FD Enabled:
 *      #2 RRS = 1
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test cases.
 * 
 * Response:
 *  The IUT shall not generate any error flag in this test frame.
 *  The IUT shall acknowledge the test frame.
 *  The data received by the IUT during the test state shall match the data
 *  sent in the test frame.
 * 
 * @todo: Classical CAN version not supported!
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

class TestIso_7_1_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::ClassicalAndFdEnabled);
            if (test_variants.size() > 0)
            {
                if (test_variants[0] == TestVariant::Can_2_0)
                    AddElemTest(TestVariant::Can_2_0, ElementaryTest(1, FrameType::Can2_0));
                else
                    AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
            }

            CanAgentConfigureTxToRxFeedback(true);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, IdentifierType::Base);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force bit as given by elementary test to recessive.
             *   2. Update frames since by sending different bit value, CRC might have changed.
             *   3. Monitor frame as if received (IUT is receiving)
             *************************************************************************************/
            if (test_variant == TestVariant::Can_2_0)
            {
                /* When node is "Classical CAN" conformant, it shall accept recessive R0 (FDF) and
                 * continue without protocol exception or regarding this frame type as FD Frame!!
                 */
                driver_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
                monitor_bit_frm->GetBitOf(0, BitType::R0)->bit_value_ = BitValue::Recessive;
            } else {
                /* R1 bit corresponds to RRS in CAN FD frames. It is bit on position of RTR in
                 * CAN2.0 frames!
                 */
                driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
            }

            driver_bit_frm->UpdateFrame();
            monitor_bit_frm->UpdateFrame();

            monitor_bit_frm->TurnReceivedFrame();

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            FreeTestObjects();
            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};