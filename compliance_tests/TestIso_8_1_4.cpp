/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 24.10.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.4
 *
 * @brief This test verifies the capacity of the IUT to manage the arbitration
 *        mechanism on every bit position in an extended format frame it is
 *        transmitting.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD Enabled
 *
 * Test variables:
 *      ID
 *      DLC
 *      FDF = 0
 *
 * Elementary test cases:
 *      For an OPEN device, there are, at most, 31 elementary tests to perform.
 *          Transmitted frame
 *    ID      RTR/RRS         DATA      Description of the     Number of elementary
 *                           field     concerned arbitration          tests
 *                                            bit               
 *  0x1FBFFFFF   0           No Data    Collision on all bits           28
 *                            field         equal to 1.
 *  0x00400000   0           No Data    Collision on all bits           1
 *                            field         equal to 1.
 *  0x00400000   0           No Data    Collision on SRR and            2
 *                            field         IDE bit
 * 
 *  For a SPECIFIC device, all possible possibilities of transmitting a recessive
 *  arbitration bit shall be considered.
 *  
 *  For CAN FD enabled test, the RTR is represented by RRS and transmitted as 0.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame. Then, the LT forces a recessive
 *  bit in the arbitration field to the dominant state according to the table in
 *  elementary test cases and continues to send a valid frame.
 *
 * Response:
 *  The IUT shall become receiver when sampling the dominant bit sent by the LT.
 *  As soon as the bus is idle, the IUT shall restart the transmission of the
 *  frame. The IUT shall not generate any error flag during the test.
 *  The content of the frame shall match the LT request.
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

class TestIso_8_1_4 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 31; i++) {
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));
            }

            /* Basic setup for tests where IUT transmits */
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            uint8_t dlc = 0x1;
            int id_iut;
            int id_lt;
            RtrFlag rtr_flag;

            /* We match bit position to be flipped with test index. Last two tests are tests where
             * we test on SRR or IDE
             */
            if (elem_test.index == 7 || elem_test.index == 30 || elem_test.index == 31)
                id_iut = 0x00400000;
            else
                id_iut = 0x1FBFFFFF; 

            /* LT must have n-th bit of ID set to dominant */
            id_lt = id_iut;
            if (elem_test.index < 30)
                id_lt &= ~(1 << (29 - elem_test.index));

            /* On elementary test 31, LT send base frame. Correct the ID so that LT sends base ID
             * with the same bits as IUT. This will guarantee that IUT sending extended frame with
             * 0x00400000 will send the same first bits as LT. Since LT will be sending base frame,
             * but IUT extended frame, IUT will loose on IDE bit!
            */
            else if (elem_test.index == 31)
                id_lt = (id_iut >> 18) & 0x7FF;

            /* On elementary test 31, IUT shall loose on IDE bit. Occurs when LT sends base frame! */
            IdentifierType ident_type_lt;
            if (elem_test.index == 31)
                ident_type_lt = IdentifierType::Base;
            else
                ident_type_lt = IdentifierType::Extended;

            /* On elementary test 31, IUT shall loose on IDE bit. After Base ID IUT will send
             * recessive SRR. LT must also send recessive bit (RTR) otherwise IUT would loose on
             * SRR and not IDE!
             */
            if (elem_test.index == 31)
                rtr_flag = RtrFlag::RtrFrame;
            else
                rtr_flag = RtrFlag::DataFrame;

            /* In this test, we MUST NOT shift bit-rate! After loosing arbitration, IUT will
             * resynchronize in data bit-rate if granularity of data bit-rate is higher than
             * of nominal bit-rate! This would result in slightly shifted monitored frame
             * compared to IUT!
             */
            frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type, ident_type_lt,
                                    rtr_flag, BrsFlag::DontShift, EsiFlag::ErrorActive);
            frame_flags_2 = std::make_unique<FrameFlags>(elem_test.frame_type,
                                    IdentifierType::Extended, rtr_flag, BrsFlag::DontShift,
                                    EsiFlag::ErrorActive);

            golden_frm = std::make_unique<Frame>(*frame_flags, dlc, id_lt);
            RandomizeAndPrint(golden_frm.get());
            
            /* This frame is actually given to IUT to send it */
            golden_frm_2 = std::make_unique<Frame>(*frame_flags_2, dlc, id_iut);
            RandomizeAndPrint(golden_frm_2.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            driver_bit_frm_2 = ConvertBitFrame(*golden_frm_2);
            monitor_bit_frm_2 = ConvertBitFrame(*golden_frm_2);

            /**************************************************************************************
             * Modify test frames:
             *   1. Force n-th bit of monitored frame to recessive. Monitored frame is created from
             *      golden_frame which has n-th bit dominant, but IUT is requested to send frame
             *      with this bit recessive (golden_frm_2). Therefore this bit shall be expected
             *      recessive. Bit position is calculated from elementary test index. First 11
             *      tests are in base identifier, next 18 are in Identifier extension.
             *   2. Loose arbitration on n-th bit of base identifier in monitored frame. Skip stuff
             *      bits!
             *   3. Append second frame as if retransmitted by IUT. This one must be created from
             *      frame which was actually issued to IUT
             *************************************************************************************/
            Bit *loosing_bit;

            if (elem_test.index < 12){
                loosing_bit = monitor_bit_frm->GetBitOfNoStuffBits(elem_test.index - 1,
                                                        BitType::BaseIdentifier);
            } else if (elem_test.index < 30){
                loosing_bit = monitor_bit_frm->GetBitOfNoStuffBits(elem_test.index - 12,
                                                        BitType::IdentifierExtension);
            /* Elementary test 30 - loose on SRR */
            } else if (elem_test.index == 30) {
                loosing_bit = monitor_bit_frm->GetBitOf(0, BitType::Srr);

            /* Elementary test 31 - loose on IDE */
            } else if (elem_test.index == 31){
                loosing_bit = monitor_bit_frm->GetBitOf(0, BitType::Ide);
            }

            loosing_bit->bit_value_ = BitValue::Recessive;

            monitor_bit_frm->LooseArbitration(loosing_bit);

            /* On elementary test 30, IUT shall loose on SRR bit, therefore we must send this bit
             * dominant by LT, so we flip it
             */
            if (elem_test.index == 30){
                Bit *srr_bit = driver_bit_frm->GetBitOf(0, BitType::Srr);
                srr_bit->bit_value_ = BitValue::Dominant;
                int index = driver_bit_frm->GetBitIndex(srr_bit);

                /* Forcing SRR low will cause 5 consecutive dominant bits at the end of base ID,
                 * therefore IUT inserts recessive stuff bit. Model does not account for this,
                 * so we must insert one extra bit in monitored frame. For driven frame, we must
                 * recalculate CRC!
                 */
                driver_bit_frm->UpdateFrame();
                monitor_bit_frm->InsertBit(BitType::Srr, BitValue::Recessive, index + 1);
            }

            /* On elementary test 31, IUT will be sending extended frame with the same base ID as
             * LT. LT will be sending Base frame. But monitored frame is constructed from LTs
             * frame which always has RTR bit dominant (right after Base ID). IUT is sending
             * Extended frame, therefore at position of RTR it will send SRR which is Recessive.
             * So bit at position of RTR in monitored frame must be set to recessive!
             * 
             * Note that in CAN FD frame there is no RTR bit so R1 must be set instead!
             */
            if (elem_test.index == 31) {
                if (test_variant == TestVariant::Common)
                    monitor_bit_frm->GetBitOf(0, BitType::Rtr)->bit_value_ = BitValue::Recessive;
                else
                    monitor_bit_frm->GetBitOf(0, BitType::R1)->bit_value_ = BitValue::Recessive;
            }

            driver_bit_frm_2->TurnReceivedFrame();
            driver_bit_frm->AppendBitFrame(driver_bit_frm_2.get());
            monitor_bit_frm->AppendBitFrame(monitor_bit_frm_2.get());

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            this->dut_ifc->SendFrame(golden_frm_2.get());                    
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();
            CheckRxFrame(*golden_frm);

            FreeTestObjects();
            return FinishElementaryTest();
        }
    
        ENABLE_UNUSED_ARGS
};