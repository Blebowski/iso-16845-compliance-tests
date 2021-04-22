/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 4.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.1.8
 * 
 * @brief This test verifies the behaviour of the IUT when receiving a correct
 *        classical frame with a DLC greater than 8.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  DLC, FDF = 0
 * 
 * Elementary test cases:
 *  There are seven elementary tests, for which DLC ∈ [9h , Fh].
 * 
 *      TEST    DLC
 *       #1     0x9
 *       #2     0xA
 *       #3     0xB
 *       #4     0xC
 *       #5     0xD
 *       #6     0xE
 *       #7     0xF
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test cases.
 * 
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall acknowledge the test frame.
 *  The data and DLC received by the IUT during the test state shall match the
 *  data and DLC sent in the test frame.
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

class TestIso_7_1_8 : public test_lib::TestBase
{
    public:

        uint8_t dlcs[7] = {0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            for (int i = 0; i < 7; i++)
                 AddElemTest(TestVariant::Common, ElementaryTest(i + 1));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(FrameType::Can2_0);
            golden_frm = std::make_unique<Frame>(*frame_flags, dlcs[elem_test.index - 1]);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received, driver frame must have ACK too (TX->RX feedback
             *      disabled).
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

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
};