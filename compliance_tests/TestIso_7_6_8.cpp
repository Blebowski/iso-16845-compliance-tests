/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.8
 * 
 * @brief This test verifies that the IUT increases its REC by 1 when detecting
 *        a form error on the EOF field during reception of a data frame.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      REC, FDF = 0
 * 
 *  CAN FD Enabled
 *      REC, FDF = 1
 * 
 * Elementary test cases:
 *      #1 corrupting the second bit of the EOF;
 *      #2 corrupting the third bit of the EOF;
 *      #3 corrupting the fifth bit of the EOF.
 *
 * Setup:
 *  The IUT is left in the default state.
 *  The LT sends a frame with a stuff error in it and force 1 bit of error flag
 *  to recessive.
 *  This initializes the REC counter to 1 + 8 REC = 9.
 * 
 * Execution:
 *  The LT sends a frame with the EOF modified according to elementary test
 *  cases.
 *
 * Response:
 *  The REC value shall be decreased by 1 because the frame is error free until
 *  ACK.
 *  The REC value shall be increased by 1 on the replaced bit of the EOF.
 *  The REC value shall be unchanged as previous initialized while set up.
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

class TestIso_7_6_8 : public test_lib::TestBase
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
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip n-th bit of EOF to DOMINANT
             *   3. Insert expected Active error frame from next bit of EOF!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            int bit_to_corrupt;
            if (elem_test.index == 1)
                bit_to_corrupt = 2;
            else if (elem_test.index == 2)
                bit_to_corrupt = 3;
            else
                bit_to_corrupt = 5;
            TestMessage("Forcing EOF bit %d to Dominant", bit_to_corrupt);
            driver_bit_frm->GetBitOf(bit_to_corrupt - 1, BitType::Eof)->bit_value_ =
                BitValue::Dominant;

            driver_bit_frm->InsertActiveErrorFrame(bit_to_corrupt, BitType::Eof);
            monitor_bit_frm->InsertActiveErrorFrame(bit_to_corrupt, BitType::Eof);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /*************************************************************************************
             * Execute test
             ************************************************************************************/
            /* Dont use extra frame, but preset REC directly -> Simpler */
            dut_ifc->SetRec(9);
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);

            CheckLowerTesterResult();
            CheckRecChange(rec_old, +0);

            FreeTestObjects();
            return FinishElementaryTest();
        }
        ENABLE_UNUSED_ARGS
};