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
 * @date 15.11.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 7.7.2
 * 
 * @brief The purpose of this test is to verify that the IUT makes a hard
 *        synchronization when receiving an early SOF delayed by e,
 *        e ∈ [1, NTQ(N)].
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables:
 *  Sampling_Point(N) configuration as available by IUT.
 *      FDF = 0
 * 
 * Elementary test cases:
 *      There is one elementary test to perform for each possible value of e.
 *          #1 Length of third bit of intermission field is e ∈ [1, NTQ(N)].
 *      Refer to 6.2.3.
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  The LT sends a first test frame disturbed by an error frame and after the
 *  second bit of the intermission field, it sends an SOF delayed by e time
 *  quanta depending on the elementary test cases.
 *  The SOF is followed by a sequence of 5 dominant bits.
 * 
 * Response:
 *  The IUT shall respond with an error frame 6 bit times − 1TQ(N) (Sync_Segment)
 *  or up to 6 bit times after the recessive to dominant edge at the beginning
 *  of the SOF.
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

class TestIso_7_7_2 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::Common);
            for (size_t i = 0; i < nominal_bit_timing.GetBitLengthTimeQuanta(); i++){
                ElementaryTest test = ElementaryTest(i + 1, FrameType::Can2_0);
                test.e_ = i + 1;
                AddElemTest(TestVariant::Common, std::move(test));
            }

            //CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentConfigureTxToRxFeedback(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type_, IdentifierType::Base);

            /* Base ID = 0x0 */
            golden_frm = std::make_unique<Frame>(*frame_flags, 0x1, 0x0);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Monitor frame as if received.
             *   2. Flip 5-th bit of Base identifier to dominant. This should cause error frame to
             *      be transmitted from next bit
             *   3. Insert Active Error frame to monitored frame. Insert Passive Error frame to
             *      driven frame.
             *   4. Shorten last bit of intermission by NTQ - e. This leaves third bit of
             *      intermission with lenght of only e.
             *   5. Turn second monitored frame as if received.
             *   6. In second frame, force 5-th bit of Base identifier to dominant.
             *   7. In second frame, insert active error frame from 6-th bit of Base Identifier to
             *      monitored frame. Insert Passive error frame to driven frame.
             *   8. Append second frame to first frame.
             *************************************************************************************/
            monitor_bit_frm->TurnReceivedFrame();

            driver_bit_frm->GetBitOf(4, BitType::BaseIdentifier)->FlipBitValue();

            driver_bit_frm->InsertPassiveErrorFrame(5, BitType::BaseIdentifier);
            monitor_bit_frm->InsertActiveErrorFrame(5, BitType::BaseIdentifier);

            /* Clear all phases and keep only SYNC. Then lengthen SYNC accordingly! */
            Bit *last_interm_bit_drv = driver_bit_frm->GetBitOf(2, BitType::Intermission);
            Bit *last_interm_bit_mon = monitor_bit_frm->GetBitOf(2, BitType::Intermission);

            last_interm_bit_drv->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            last_interm_bit_drv->ShortenPhase(BitPhase::Ph1, nominal_bit_timing.ph1_);
            last_interm_bit_drv->ShortenPhase(BitPhase::Prop, nominal_bit_timing.prop_);

            last_interm_bit_mon->ShortenPhase(BitPhase::Ph2, nominal_bit_timing.ph2_);
            last_interm_bit_mon->ShortenPhase(BitPhase::Ph1, nominal_bit_timing.ph1_);
            last_interm_bit_mon->ShortenPhase(BitPhase::Prop, nominal_bit_timing.prop_);

            last_interm_bit_drv->LengthenPhase(BitPhase::Sync, elem_test.e_ - 1);
            last_interm_bit_mon->LengthenPhase(BitPhase::Sync, elem_test.e_ - 1);

            monitor_bit_frm_2->TurnReceivedFrame();

            driver_bit_frm_2->GetBitOf(4, BitType::BaseIdentifier)->FlipBitValue();

            driver_bit_frm_2->InsertPassiveErrorFrame(5, BitType::BaseIdentifier);
            monitor_bit_frm_2->InsertActiveErrorFrame(5, BitType::BaseIdentifier);

            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /***************************************************************************** 
             * Execute test
             *****************************************************************************/
            dut_ifc->SetRec(0);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            RunLowerTester(true, true);
            CheckLowerTesterResult();
            /* No frame shall be received by IUT since both had errors in it! */
            CheckNoRxFrame();

            return FinishElementaryTest();
        }
};