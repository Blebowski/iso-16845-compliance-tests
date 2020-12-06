/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 29.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.5.6
 * 
 * @brief The purpose of this test is to verify that an error passive IUT
 *        detects a form error when receiving an invalid error delimiter.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Error delimiter of passive error frame, FDF = 0
 * 
 *  CAN FD Enabled
 *      Error delimiter of passive error frame, FDF = 1
 * 
 * Elementary test cases:
 *  Elementary tests to perform:
 *      #1 corrupting the second bit of the error delimiter;
 *      #2 corrupting the fourth bit of the error delimiter;
 *      #3 corrupting the seventh bit of the error delimiter.
 *
 * Setup:
 *  The IUT is set in passive state.
 * 
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  During the error delimiter, the LT creates a form error according to
 *  elementary test cases.
 *  After the form error, the LT waits for (6 + 7) bit time before sending
 *  a dominant bit, corrupting the last bit of the error delimiter.
 * 
 * Response:
 *  The IUT shall generate an overload frame starting at the bit position
 *  following the last dominant bit sent by the LT.
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

class TestIso_7_5_6 : public test_lib::TestBase
{
    public:
        
        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 3; i++)
            {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            dut_ifc->SetTec((rand() % 110) + 128);
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                            IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                            EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &error_data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff bit
             *      therefore causing error.
             *   3. Insert Passive Error frame to both driven and monitored frames from next bit.
             *   4. Corrupt 2/4/7-th bit of Error delimiter to dominant on driven frame.
             *   5. Insert next error frame from next bit on. Both driven and monitored frames
             *      contain passive error frame.
             *   6. Flip last bit (8-th) of error delimiter of new error frame to dominant.
             *   7. Insert overload frame to both driven and monitored frames (TX/RX feedback is
             *      disabled).
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            int bit_to_corrupt;
            if (elem_test.index == 1)
                bit_to_corrupt = 1;
            else if (elem_test.index == 2)
                bit_to_corrupt = 3;
            else
                bit_to_corrupt = 6;

            Bit *corrupted_bit = driver_bit_frm->GetBitOf(bit_to_corrupt,
                                    BitType::ErrorDelimiter);
            int bit_index = driver_bit_frm->GetBitIndex(corrupted_bit);
            corrupted_bit->bit_value_ = BitValue::Dominant;

            driver_bit_frm->InsertPassiveErrorFrame(bit_index + 1);
            monitor_bit_frm->InsertPassiveErrorFrame(bit_index + 1);

            /* This should be last bit of second Error delimiter*/
            driver_bit_frm->GetBit(bit_index + 14)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->InsertOverloadFrame(bit_index + 15);
            monitor_bit_frm->InsertOverloadFrame(bit_index + 15);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            CheckNoRxFrame();
 
            FreeTestObjects();
            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};