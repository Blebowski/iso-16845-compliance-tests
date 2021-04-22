/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 28.9.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.5.1
 * 
 * @brief The purpose of this test is to verify that an error passive IUT
 *        considers the passive error flag as completed after the detection
 *        of 6 consecutive bits of the same value.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Classical CAN, CAN FD Tolerant, CAN FD Enabled
 *      Passive Error flag, FDF = 0
 * 
 *  CAN FD Enabled
 *      Passive Error flag, FDF = 1
 * 
 * Elementary test cases:
 *  Elementary tests to perform:
 *      #1 superimposing the passive error flag by an active error flag
 *         starting at the first bit;
 *      #2 superimposing the passive error flag by an active error flag
 *         starting at the third bit;
 *      #3 superimposing the passive error flag by an active error flag
 *         starting at the sixth bit.
 *
 * Setup:
 *  The IUT is set in passive state.
 * 
 * Execution:
 *  The LT causes the IUT to generate a passive error frame in data field.
 *  During the passive error flag sent by the IUT, the LT sends an active
 *  error flag according to elementary test cases.
 *  At the end of the active error flag, the LT waits for (8 + 2) bit time
 *  before sending a valid test frame.
 * 
 * Response:
 *  The IUT shall acknowledge the test frame.
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

class TestIso_7_5_1 : public test_lib::TestBase
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

            dut_ifc->SetTec(140);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                            IdentifierType::Base, RtrFlag::DataFrame, BrsFlag::DontShift,
                            EsiFlag::ErrorPassive);
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, &error_data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 7-th bit of data field to dominant. This should be recessive stuff bit
             *      therefore causing error.
             *   3. Insert Passive Error frame to both driven and monitored frames from next bit.
             *   4. Superimpose active error flag on driven frame starting from 1/3/6th bit of
             *      Passive Error flag. On monitored frame, insert passive error frame again. This
             *      corresponds prolonging passive error flag until sequence of equal consecutive
             *      bits is received!
             *   5. Remove last bit of intermission in driven frame. This corresponds to +2 bits
             *      separation in test description.
             *   6. Turn monitored frame as if received, remove SOF since frame is transmitted by
             *      LT after second bit of intermission.
             *   7. Append the second frame to original frame. Second driven frame must have ACK
             *      dominant since TX/RX feedback is disabled!
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(6, BitType::Data)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);
            monitor_bit_frm->InsertPassiveErrorFrame(7, BitType::Data);

            int bit_to_corrupt;
            if (elem_test.index == 1)
                bit_to_corrupt = 0;
            else if (elem_test.index == 2)
                bit_to_corrupt = 2;
            else
                bit_to_corrupt = 5;

            driver_bit_frm->InsertActiveErrorFrame(bit_to_corrupt, BitType::PassiveErrorFlag);
            monitor_bit_frm->InsertPassiveErrorFrame(bit_to_corrupt, BitType::PassiveErrorFlag);

            driver_bit_frm->RemoveBit(2, BitType::Intermission);

            monitor_bit_frm_2->TurnReceivedFrame();
            monitor_bit_frm_2->RemoveBit(0, BitType::Sof);
            driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            
            CheckRxFrame(*golden_frm);
            CheckNoRxFrame(); /* Only one frame should be received */
            
            FreeTestObjects();
            return FinishElementaryTest();
        }
};