/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 18.7.2020
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * @test ISO16845 8.1.6
 *
 * @brief The purpose of this test is to verify that an IUT correctly generates
 *        the stuff bits in a base format frame.
 *
 * @version Classical CAN, CAN FD tolerant, CAN FD enabled
 *
 * Test variables:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          ID, RTR, DATA, DLC, FDF = 0
 *      CAN FD enabled:
 *          ID, RRS, BRS, ESI, DLC, DATA, FDF = 1
 *
 * Elementary test cases:
 *      Classical CAN, CAN FD tolerant, CAN FD enabled:
 *          For an OPEN device, there are six elementary tests to perform:
 *                          CBFF
 *              ID          CTRL        DATA
 *       #1    0x78         0x08      first byte: 0x01, others: 0xE1
 *       #2   0x41F         0x01      0x00
 *       #3   0x47F         0x01      0x1F
 *       #4   0x758         0x00       -
 *       #5   0x777         0x01      0x1F
 *       #6   0x7EF         0x42       -
 *
 *      CAN FD Enabled:
 *          The following cases are tested:
 *                          FBFF
 *             ID           CTRL        DATA     
 *       #1    0x78        0x0AE      0xF8, all other bytes 0x78
 *       #2   0x4C7        0x0A8      all bytes 0x3C
 *       #3   0x41E        0x0BE      all bytes 0x1E
 *       #4   0x20F        0x09F      all bytes 0x0F
 *       #5   0x107        0x08F      all bytes 0x87
 *       #6   0x7C3        0x083      all bytes 0xC3
 *       #7   0x3E1        0x0A3      all bytes 0xE1
 *       #8   0x1F0        0x0A1      all bytes 0xF0
 *       #9   0x000        0x0A0      -
 *      #10   0x7FF                   0xB0
 *          There are 10 elementary tests to perform.
 * 
 * Setup:
 *  The IUT is left in the default state.
 *
 * Execution:
 *  The LT causes the IUT to transmit a frame according to elementary test
 *  cases.
 *
 * Response:
 *  The IUT shall not generate any error flag during the test.
 *  The IUT shall correctly generate all stuff bits.
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

class TestIso_8_1_6 : public test_lib::TestBase
{
    public:

        void ConfigureTest()
        {
            FillTestVariants(VariantMatchingType::CommonAndFd);
            for (int i = 0; i < 6; i++)
                AddElemTest(TestVariant::Common, ElementaryTest(i + 1, FrameType::Can2_0));
            for (int i = 0; i < 10; i++)
                AddElemTest(TestVariant::CanFdEnabled, ElementaryTest(i + 1, FrameType::CanFd));

            /* Basic setup for tests where IUT transmits */
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetWaitForMonitor(true);
            CanAgentConfigureTxToRxFeedback(true);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
        }

        DISABLE_UNUSED_ARGS

        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            if (test_variant == TestVariant::Common)
            {
                /* Last iteration (0x42 CTRL field) indicates RTR frame */
                RtrFlag rtr_flag;
                if (elem_test.index == 6)
                    rtr_flag = RtrFlag::RtrFrame;
                else
                    rtr_flag = RtrFlag::DataFrame;

                frame_flags = std::make_unique<FrameFlags>(elem_test.frame_type,
                                        IdentifierType::Base, rtr_flag);

                /* Data, DLCs and identifiers for each iteration */
                uint8_t data[6][8] = {
                    {0x01, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
                };
                uint8_t dlcs[6] = {
                    0x8, 0x1, 0x1, 0x0, 0x1, 0x2
                };
                int ids[6] = {
                    0x78, 0x41F, 0x47F, 0x758, 0x777, 0x7EF
                };
                golden_frm = std::make_unique<Frame>(*frame_flags, dlcs[elem_test.index - 1],
                                    ids[elem_test.index - 1], data[elem_test.index -1]);

            } else if (test_variant == TestVariant::CanFdEnabled) {
                
                /* Flags based on elementary test */
                switch(elem_test.index)
                {
                    case 1:
                    case 2:
                    case 7:
                    case 8:
                    case 9:
                        frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorActive);
                        break;

                    case 3:
                        frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::Shift, EsiFlag::ErrorPassive);
                        break;

                    case 4:
                        frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorPassive);
                        break;

                    case 5:
                    case 6:
                        frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame,
                                            BrsFlag::DontShift, EsiFlag::ErrorActive);
                        break;
                    
                    case 10:
                        frame_flags = std::make_unique<FrameFlags>(FrameType::CanFd,
                                            IdentifierType::Base, RtrFlag::DataFrame);
                        break;
                    default:
                        break;
                };

                /* DUT must be set to error passive state when ErrorPassive is expected!
                    * Otherwise, it would transmitt ESI_ERROR_ACTIVE
                    */
                if (elem_test.index == 3 || elem_test.index == 4)
                    dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);
                else
                    dut_ifc->SetErrorState(FaultConfinementState::ErrorActive);

                int ids[10] = {
                    0x78, 0x47C, 0x41E, 0x20F, 0x107, 0x7C3, 0x3E1, 0x1F0, 0x000, 0x7FF
                };

                /* Data based on elementary test */
                uint8_t data[10][64] = {
                    {
                        0xF8, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
                        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F
                    },
                    {
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 
                        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87
                    },
                    {
                        0xC3, 0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    { // Dont care since DLC = 0
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    },
                    {
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0,
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 
                        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0
                    }
                };

                uint8_t dlcs[10] = {
                    0xE, 0x8, 0xE, 0xF, 0xF, 0x3, 0x3, 0x1, 0x0, (uint8_t)(rand() % 0xF)
                };
                golden_frm = std::make_unique<Frame>(*frame_flags, dlcs[elem_test.index - 1],
                                    ids[elem_test.index - 1], data[elem_test.index - 1]);
            }

            /* Randomize will have no effect since everything is specified */
            RandomizeAndPrint(golden_frm.get());

            driver_bit_frm = ConvertBitFrame(*golden_frm);
            monitor_bit_frm = ConvertBitFrame(*golden_frm);

            /**************************************************************************************
             * Modify test frames:
             *   1. Turn driven frame as if received (insert ACK).
             * 
             * No other modifications are needed as correct stuff generation is
             * verified by model!
             **************************************************************************************/
            driver_bit_frm->TurnReceivedFrame();

            driver_bit_frm->Print(true);
            monitor_bit_frm->Print(true);

            /**************************************************************************************
             * Execute test
             *************************************************************************************/
            PushFramesToLowerTester(*driver_bit_frm, *monitor_bit_frm);
            StartDriverAndMonitor();
            this->dut_ifc->SendFrame(golden_frm.get());
            WaitForDriverAndMonitor();
            CheckLowerTesterResult();
            
            FreeTestObjects();
            return FinishElementaryTest();
        }
    
        ENABLE_UNUSED_ARGS
};