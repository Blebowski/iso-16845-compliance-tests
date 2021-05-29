/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 17.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.7
 * 
 * @brief This test verifies that the IUT increases its REC by 1 when detecting
 *        a form error on ACK delimiter.
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
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      #1 Classical CAN frame (ACK = 0, ACK delimiter = 0)
 * 
 *  CAN FD Enabled
 *    In FD format, 2 ACK bits are used to check the second possible ACK bit
 *    position (delayed ACK).
 *      #1 FD frame (ACK bit 1 = 0; ACK bit 2 = 0, ACK delimiter = 0)
 *
 * Setup:
 *  The IUT is left in the default state.
 *  The LT sends a frame with a stuff error in it and force 1 bit of error flag
 *  to recessive.
 *  This initializes the REC counter to 1 + 8 REC = 9.
 * 
 * Execution:
 *  The LT sends a frame according to elementary test cases.
 *
 * Response:
 *  The IUT’s REC value shall be increased by 1 on the dominant ACK delimiter.
 *  The REC value shall be decreased by 1 because the frame is error free until
 *  ACK.
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

class TestIso_7_6_7 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Set ACK bit to dominant in driven frame. In CAN FD Enabled variant, flip both
             *      ACK bits to dominant!
             *   3. Flip ACK delimiter in driven frame (on can_tx) to DOMINANT!
             *   4. Insert expected Active error frame from first bit of EOF!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
            if (test_variant == TestVariant::CanFdEnabled)
                driver_bit_frm->GetBitOf(1, BitType::Ack)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->GetBitOf(0, BitType::AckDelimiter)->bit_value_ = BitValue::Dominant;

            driver_bit_frm->InsertActiveErrorFrame(0, BitType::Eof);
            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Eof);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
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
};