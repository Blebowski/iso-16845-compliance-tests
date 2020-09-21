/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 6.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.2.7
 * 
 * @brief This test verifies that the IUT detects a form error when the
 *        recessive bit of CRC delimiter is forced to dominant state by LT.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      CRC Delimiter, FDF = 0
 * 
 *  CAN FD Enabled
 *      CRC Delimiter, FDF = 1
 * 
 * Elementary test cases:
 *      #1 CRC Delimiter = 0
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for the elementary test.
 *  The LT generates a CAN frame with form error at CRC delimiter according to
 *  elementary test cases.
 * 
 * Response:
 *  The IUT shall generate an active error frame at the bit position following
 *  the CRC delimiter
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

class TestIso_7_2_7 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            num_elem_tests = 1;
            elem_tests[0].push_back(ElementaryTest(1, FrameType::Can2_0));
            elem_tests[1].push_back(ElementaryTest(1, FrameType::CanFd));
        }

        int Run()
        {
            SetupTestEnvironment();

            for (size_t test_variant = 0; test_variant < test_variants.size(); test_variant++)
            {
                PrintVariantInfo(test_variants[test_variant]);

                frame_flags = std::make_unique<FrameFlags>(elem_tests[test_variant][0].frame_type);
                golden_frm = std::make_unique<Frame>(*frame_flags);
                RandomizeAndPrint(golden_frm.get());

                driver_bit_frm = ConvertBitFrame(*golden_frm);
                monitor_bit_frm = ConvertBitFrame(*golden_frm);
            
                /**********************************************************************************
                 * Modify test frames:
                 *   1. Monitor frame as if received.
                 *   2. Force CRC Delimiter to Dominant in driven frame.
                 *   3. Insert Active Error frame from ACK delimiter ON in both
                 *      driven/monitored frame.
                 **********************************************************************************/
                monitor_bit_frm->TurnReceivedFrame();

                driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter)->bit_value_ = BitValue::Dominant;
                
                monitor_bit_frm->InsertActiveErrorFrame(
                    monitor_bit_frm->GetBitOf(0, BitType::Ack));
                driver_bit_frm->InsertActiveErrorFrame(
                    driver_bit_frm->GetBitOf(0, BitType::Ack));

                /********************************************************************************** 
                 * Execute test
                 *********************************************************************************/
                PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
                RunLowerTester(true, true);
                CheckLowerTesterResult();
            }

            return (int)FinishTest();
        }
};