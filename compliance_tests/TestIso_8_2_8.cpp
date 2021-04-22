/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 30.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * @test ISO16845 8.2.8
 * 
 * @brief This test verifies that the IUT detects an error when after the
 *        transmission of 5 identical bits, it receives a sixth bit identical
 *        to the five precedents.
 * @version Classical CAN, CAN FD Tolerant, CAN FD Enabled
 * 
 * Test variables: 
 *  CAN FD Enabled
 *      Data byte 0 - 63, ID = 0x555, IDE = 0, DLC = 15, FDF = 1
 * 
 * Elementary test cases:
 *  CAN FD Enabled
 *   All 1 008 stuff bit positions within the defined data bytes will be tested.
 *
 *   There are 35 elementary tests to perform.
 * 
 *                      Data byte 0                   Data bytes 1 - 63
 *      #1 to #126          0x10                            0x78        
 *    #127 to #252          0x78                            0x3C
 *    #253 to #378          0x34                            0x1E
 *    #379 to #504          0x12                            0x0F
 *    #505 to #630          0x0F                            0x87
 *    #631 to #756          0x17                            0xC3
 *    #757 to #882          0x43                            0xE1
 *    #883 to #1008         0x21                            0xF0
 *
 * Setup:
 *  The IUT is left in the default state.
 * 
 * Execution:
 *  A single test frame is used for each elementary test. In each elementary
 *  test, the LT forces another one of the stuff bits to its complement.
 *  
 * Response:
 *  The IUT shall generate an active error frame starting at the bit position
 *  following the bit error at stuff bit position.
 * 
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

class TestIso_8_2_8 : public test_lib::TestBase
{
    public:
        bool one_shot_enabled;

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CanFdEnabledOnly);
            for (int i = 0; i < 1008; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
            CanAgentSetWaitForMonitor(true);
            /* TX to RX feedback must be disabled since we corrupt dominant bits to Recessive */
            one_shot_enabled = dut_ifc->ConfigureOneShot(true);
        }

        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            uint8_t data[64] = {};
            uint8_t data_first;
            uint8_t data_rest;

            int stuff_bit_index;

            if (elem_test.index < 127) {
                data_first = 0x10;
                data_rest = 0x78;
                stuff_bit_index = elem_test.index - 1;
            } else if (elem_test.index < 253){
                data_first = 0x78;
                data_rest = 0x3C;
                stuff_bit_index = elem_test.index - 127;
            } else if (elem_test.index < 379){
                data_first = 0x34;
                data_rest = 0x1E;
                stuff_bit_index = elem_test.index - 253;
            } else if (elem_test.index < 505){
                data_first = 0x12;
                data_rest = 0x0F;
                stuff_bit_index = elem_test.index - 379;
            } else if (elem_test.index < 631){
                data_first = 0x0F;
                data_rest = 0x87;
                stuff_bit_index = elem_test.index - 505;
            } else if (elem_test.index < 757){
                data_first = 0x17;
                data_rest = 0xC3;
                stuff_bit_index = elem_test.index - 631;
            } else if (elem_test.index < 883){
                data_first = 0x43;
                data_rest = 0xE1;
                stuff_bit_index = elem_test.index - 757;
            } else {
                stuff_bit_index = elem_test.index - 883;
                data_first = 0x21;
                data_rest = 0xF0;
            }

            data[0] = data_first;
            for (int i = 1; i < 64; i++)
                data[i] = data_rest;

            frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd, IdentifierType::Base,
                                        RtrFlag::DataFrame, BrsFlag::Shift, EsiFlag::ErrorActive);

            golden_frm = std::make_unique<Frame>(*frame_flags, 0xF, 0x555, data);
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Choose stuff bit as given by elementary test. Description of elementary tests
             *      should match number of stuff bits (e.g. in first frame 126 stuff bits)!
             *   2. Corrupt stuff bit from point 1 to opposite value.
             *   3. Insert Active Error frame from next bit on.
             *   4. Append retransmitted frame if one shot mode is not enabled. If it is enabled,
             *      IUT will not retransmitt the frame. This serves to shorten the test time!
             *************************************************************************************/
            int num_stuff_bits = driver_bit_frm->GetNumStuffBits(BitType::Data,
                                    StuffBitType::NormalStuffBit);
            Bit *stuff_bit;

            /* It can be that last bit is right after last bit of data!! */
            if (num_stuff_bits > stuff_bit_index)
                stuff_bit = driver_bit_frm->GetStuffBit(stuff_bit_index, BitType::Data);
            else
                stuff_bit = driver_bit_frm->GetStuffBit(0, BitType::StuffCount);

            stuff_bit->FlipBitValue();
            int bit_index = driver_bit_frm->GetBitIndex(stuff_bit);

            driver_bit_frm->InsertActiveErrorFrame(bit_index + 1);
            monitor_bit_frm->InsertActiveErrorFrame(bit_index + 1);

            if (!one_shot_enabled)
            {
                driver_bit_frm_2->GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
                driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
                monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());
            }

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /*****************************************************************************
             * Execute test
             ****************************************************************************/
            dut_ifc->SetTec(0);
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();

            driver_bit_frm.reset();
            monitor_bit_frm.reset();
            driver_bit_frm_2.reset();
            monitor_bit_frm_2.reset();

            return FinishElementaryTest();
        }
    
};