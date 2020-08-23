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
 * @test ISO16845 7.1.5
 * 
 * @brief The purpose of this test is to verify that the IUT accepts the
 *        non-nominal value of bit described in test variables in a valid
 *        extended format frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN   : SRR, FDF, r0
 *  CAN FD Tolerant : SRR, FDF, r0=0
 *  CAN FD Enabled  : SRR, RRS, FDF=1
 * 
 * Elementary test cases:
 *  Classical CAN:
 *      TEST    SRR     r0      FDF
 *       #1      1       1       1
 *       #2      1       1       0
 *       #3      1       0       1
 *       #4      0       1       1
 *       #5      0       1       0
 *       #6      0       0       1
 *       #7      0       0       0   
 * 
 *  CAN FD Tolerant:
 *      TEST    SRR     r0
 *       #1      1       1
 *       #2      0       1
 *       #3      0       0
 *
 *  CAN FD Enabled:
 *      TEST    SRR     RRS
 *       #1      1       1
 *       #2      0       1
 *       #3      0       0
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
 * @todo Only CAN FD Enabled so far implemented!
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

class TestIso_7_1_5 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::OneToOne);
            
            switch (test_variants[0])
            {
            case TestVariant::CanFdEnabled:
                num_elem_tests = 3;
                break;
            case TestVariant::CanFdTolerant:
                num_elem_tests = 3;
                break;
            case TestVariant::Can_2_0:
                num_elem_tests = 7;
                break;
            default:
                break;
            }
            for (int i = 0; i < num_elem_tests; i++)
                elem_tests[0].push_back(ElementaryTest(i + 1));
        }

        int Run()
        {
            SetupTestEnvironment();

            // TODO: Add support of CAN 2.0 and FD Tolerant
            if (test_variants[0] == TestVariant::CanFdTolerant ||
                test_variants[0] == TestVariant::Can_2_0)
                return (int)FinishTest(TestResult::Failed);


            for (auto elem_test : elem_tests[0])
            {
                PrintElemTestInfo(elem_test);

                frame_flags = std::make_unique<FrameFlags>(
                                FrameType::CanFd, IdentifierType::Extended, RtrFlag::DataFrame);
                golden_frm = std::make_unique<Frame>(*frame_flags);
                RandomizeAndPrint(*golden_frm);

                driver_bit_frm = ConvertBitFrame(*golden_frm);
                monitor_bit_frm = ConvertBitFrame(*golden_frm);

                /**************************************************************
                 * Modify test frames:
                 *   1. Force bits like so (both driver and monitor):
                 *          TEST    SRR     RRS
                 *          #1      1       1
                 *          #2      0       1
                 *          #3      0       0
                 *   2. Update frames (needed since CRC might have changed)
                 *   3. Turned monitored frame received, insert ACK to driver
                 *      (TX to RX feedback disabled)
                 **************************************************************/

                switch (elem_test.index)
                {
                case 1:
                    driver_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Recessive;
                    driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                    break;
                case 2:
                    driver_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                    driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                    monitor_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                    monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
                    break;
                case 3:
                    driver_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                    driver_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Dominant;
                    monitor_bit_frm->GetBitOf(0, BitType::Srr)->bit_value_ = BitValue::Dominant;
                    monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Dominant;
                    break;
                }

                driver_bit_frm->UpdateFrame();
                monitor_bit_frm->UpdateFrame();

                monitor_bit_frm->TurnReceivedFrame();
                driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

                /************************************************************************* 
                 * Execute test
                 *************************************************************************/
                PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                RunLowerTester(true, true);
                CheckLowerTesterResult();
                CheckRxFrame(*golden_frm);
            }

            return (int)FinishTest();
        }
};