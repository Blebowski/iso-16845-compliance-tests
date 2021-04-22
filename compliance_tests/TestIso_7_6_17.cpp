/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 26.4.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.6.17
 * 
 * @brief This test verifies that the IUT does not change the value of its REC
 *        when receiving a 13-bit length overload flag.
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
 *      #1 7 dominant bits.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  a) The test system causes a receive error to initialize the REC value to 9.
 *  b) The LT causes the IUT to generate an overload frame after a valid frame
 *     reception (REC-1).
 *     After the overload flag sent by the IUT, the LT sends a sequence
 *     according to elementary test cases.
 *
 * Response:
 *  The correct frame up to the EOF will decrement REC and the overload
 *  enlargement will not increase REC.
 *  The IUT’s REC value shall be 8.
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

class TestIso_7_6_17 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            AddElemTest(TestVariant::Common, ElementaryTest(1, FrameType::Can2_0));
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1, FrameType::CanFd));

            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type);
            golden_frm = std::make_unique<Frame>(*frame_flags);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Force last bit of EOF to DOMINANT.
             *   3. Insert expected overload frame from first bit of Intermission.
             *   4. Insert 7 Dominant bits to driver on can_tx and 7 Recessive bits to monitor on
             *      can_rx from first bit of overload delimiter.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();
            
            driver_bit_frm->GetBitOf(6, BitType::Eof)->bit_value_ = BitValue::Dominant;

            monitor_bit_frm->InsertOverloadFrame(0, BitType::Intermission);
            driver_bit_frm->InsertOverloadFrame(0, BitType::Intermission);

            Bit *overload_delim = driver_bit_frm->GetBitOf(0, BitType::OverloadDelimiter);
            int bit_index = driver_bit_frm->GetBitIndex(overload_delim);

            for (int i = 0; i < 7; i++)
            {
                driver_bit_frm->InsertBit(BitType::OverloadFlag, BitValue::Dominant, bit_index);
                monitor_bit_frm->InsertBit(BitType::OverloadFlag, BitValue::Recessive, bit_index);
            }

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            dut_ifc->SetRec(9);
            rec_old = dut_ifc->GetRec();
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);
            /* Only for sucesfull frame reception */
            CheckRecChange(rec_old, -1);

            FreeTestObjects();
            return FinishElementaryTest();
        }
};