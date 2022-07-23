/****************************************************************************** 
 * 
 * ISO16845 Compliance tests 
 * Copyright (C) 2021-present Ondrej Ille
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 * 
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 20.6.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 7.8.9.3
 *
 * @brief The purpose of this test is to verify that no edge shall be used for
 *        synchronization if the value detected at the previous sample point is
 *        the same as the bus value immediately after the edge on bit position
 *        CRC delimiter.
 *
 * @version CAN FD Enabled
 *
 * Test variables:
 *      Sampling_Point(D) and SJW(D) configuration as available by IUT.
 *      Recessive to dominant edge between 2 dominant bits.
 *          CRC delimiter
 *          FDF = 1
 *
 * Elementary test cases:
 *      There is one elementary test to perform for at least 1 bit rate
 *      configuration.
 *          #1 The LT forces the CRC delimiter bit to dominant from the second
 *             TQ until the beginning of Phase_Seg2(N).
 *
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT generates a frame with last CRC bit dominant.
 *  The LT forces the CRC delimiter bit to dominant according to elementary
 *  test cases.
 *
 * Response:
 *  The modified CRC delimiter bit shall be sampled as dominant.
 *  The frame is invalid. The CRC delimiter shall be followed by an error
 *  frame.
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

class TestIso_7_8_9_3 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(1));
            
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data_byte = 0x49;
            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, IdentifierType::Base,
                                                       RtrFlag::DataFrame, BrsFlag::Shift,
                                                       EsiFlag::ErrorActive);
            // Frame was empirically debugged to have last bit of CRC in 1!
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 50, &data_byte);
            golden_frm->Print();

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn monitor frame as if received!
             *   2. Force CRC delimiter bit to dominant from 2nd TQ till beginning of Ph2.
             *   3. Insert Active error frame to monitor from ACK bit further. Insert Passive error
             *      frame to driver bit from ACK bit further.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            Bit *crc_delimiter = driver_bit_frm->GetBitOf(0, BitType::CrcDelimiter);
            crc_delimiter->ForceTimeQuanta(1, data_bit_timing.ph1_ + data_bit_timing.prop_,
                                           BitValue::Dominant);

            driver_bit_frm->InsertPassiveErrorFrame(0, BitType::Ack);
            monitor_bit_frm->InsertActiveErrorFrame(0, BitType::Ack);

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            TestMessage("DontShift synchronisation after dominant bit sampled on CRC delimiter bit!");
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();

            return FinishElementaryTest();
        }
};