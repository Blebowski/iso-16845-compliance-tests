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

class TestIso_8_1_6 : public test_lib::TestBase
{
    public:

        int Run()
        {
            // Run Base test to setup TB
            TestBase::Run();
            TestMessage("Test %s : Run Entered", test_name);

            // Start monitoring when DUT starts transmitting!
            CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
            CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));

            // Configure driver to wait for monitor so that LT sends ACK in right moment.
            CanAgentSetWaitForMonitor(true);

            // Enable TX/RX feedback so that DUT will see its own transmitted frame!
            CanAgentConfigureTxToRxFeedback(true);

            /*****************************************************************
             * Common part of test (i=0), CAN FD enabled part of test(i=1)
             ****************************************************************/
            int iterCnt;
            FrameType dataRate;

            if (dut_can_version == CanVersion::CanFdEnabled)
                iterCnt = 2;
            else
                iterCnt = 1;

            for (int i = 0; i < iterCnt; i++)
            {
                int numElemTests;
                if (i == 0)
                {
                    TestMessage("Common part of test!");
                    numElemTests = 6;
                }
                else
                {
                    TestMessage("CAN FD enabled part of test!");
                    numElemTests = 10;
                }

                for (int j = 0; j < numElemTests; j++)
                {
                    FrameFlags frameFlags;

                    // Common part
                    if (i == 0)
                    {
                        // Last iteration (0x42 CTRL field) indicates RTR frame
                        if (j == 5)
                            frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base,
                                                    RtrFlag::RtrFrame);
                        else
                            frameFlags = FrameFlags(FrameType::Can2_0, IdentifierType::Base,
                                                    RtrFlag::DataFrame);

                        // Data, dlcs and identifiers for each iteration
                        uint8_t data[5][8] = {
                            {0x01, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
                        };
                        uint8_t dlcs[] = {
                            0x8, 0x1, 0x1, 0x0, 0x1, 0x2
                        };
                        int ids[] = {
                            0x78, 0x41F, 0x47F, 0x758, 0x777, 0x7EF
                        };

                        golden_frame = new Frame(frameFlags, dlcs[j], ids[j], data[j]);

                    // CAN FD enabled part
                    } else {
                        
                        // Flags based on elementary test
                        if (j == 0 || j == 1 || j == 6 || j == 7 || j == 8) {
                            frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                    RtrFlag::DataFrame, BrsFlag::Shift,
                                                    EsiFlag::ErrorActive);
                        } else if (j == 2) {
                            frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                    RtrFlag::DataFrame, BrsFlag::Shift,
                                                    EsiFlag::ErrorPassive);
                        } else if (j == 3) {
                            frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                    RtrFlag::DataFrame, BrsFlag::DontShift,
                                                    EsiFlag::ErrorPassive);
                        } else if (j == 4 || j == 5) {
                            frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                    RtrFlag::DataFrame, BrsFlag::DontShift,
                                                    EsiFlag::ErrorActive);
                        } else { // last is random
                            frameFlags = FrameFlags(FrameType::CanFd, IdentifierType::Base,
                                                    RtrFlag::DataFrame);
                        }

                        // DUT must be set to error passive state when ErrorPassive
                        // is expected! Otherwise, it would transmitt ESI_ERROR_ACTIVE
                        if (j == 2 || j == 3)
                            dut_ifc->SetErrorState(FaultConfinementState::ErrorPassive);
                        else
                            dut_ifc->SetErrorState(FaultConfinementState::ErrorActive);

                        int ids[] = {
                            0x78, 0x47C, 0x41E, 0x20F, 0x107,
                            0x7C3, 0x3E1, 0x1F0, 0x000, 0x7FF
                        };

                        // Data based on elementary test
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

                        uint8_t dlcs[] = {
                            0xE, 0x8, 0xE, 0xF, 0xF, 0x3, 0x3, 0x1, 0x0, (uint8_t)(rand() % 0xF)
                        };
                        golden_frame = new Frame(frameFlags, dlcs[j], ids[j], data[j]);
                    }

                    TestBigMessage("Test frame:");
                    golden_frame->Print();

                    // Convert to Bit frames
                    driver_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);
                    monitor_bit_frame = new BitFrame(*golden_frame,
                        &this->nominal_bit_timing, &this->data_bit_timing);

                    /**
                     * Modify test frames:
                     *   1. Turn driven frame as if received (insert ACK).
                     * 
                     * No other modifications are needed as correct stuff generation is
                     * verified by model!
                     */
                    driver_bit_frame->TurnReceivedFrame();

                    driver_bit_frame->Print(true);
                    monitor_bit_frame->Print(true);

                    // Push frames to Lower tester, insert to DUT, run and check!
                    PushFramesToLowerTester(*driver_bit_frame, *monitor_bit_frame);
                    StartDriverAndMonitor();

                    TestMessage("Sending frame via DUT!");
                    this->dut_ifc->SendFrame(golden_frame);
                    TestMessage("Sent frame via DUT!");
                    
                    WaitForDriverAndMonitor();
                    CheckLowerTesterResult();

                    DeleteCommonObjects();
                }
            }

            TestControllerAgentEndTest(test_result);
            TestMessage("Test %s : Run Exiting", test_name);
            return test_result;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};