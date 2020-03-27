/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
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

class test_lib::TestDemo : public TestBase
{
    public:
        // Here put any test dependent declarations!

        /**
         *
         */
        TestDemo() : TestBase()
        {
            // Here initialize test specific variables, create test specific
            // objects.
        }


        /**
         *
         */
        int run()
        {
            // Run Base test to setup TB
            TestBase::run();

            /*****************************************************************
             * Test sequence start
             ****************************************************************/

            testMessage("TestDemo: Run Entered");

            // Write your test code here!

            FrameFlags frameFlags = FrameFlags(
                CAN_FD, EXTENDED_IDENTIFIER, DATA_FRAME,
                BIT_RATE_SHIFT, ESI_ERROR_ACTIVE);

            uint8_t data[64] =
            {
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
                0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
            };

            BitFrame driverFrame = BitFrame(frameFlags, 1, 32, &(data[0]), &this->nominalBitTiming, &this->dataBitTiming);
            BitFrame monitorFrame = BitFrame(frameFlags, 1, 32, &(data[0]), &this->nominalBitTiming, &this->dataBitTiming);

            driverFrame.print(true);

            // 
            monitorFrame.turnReceivedFrame();

            monitorFrame.print(true);

            // Insert ACK to received sequence!
            driverFrame.getBitOf(0, can::BIT_TYPE_ACK)->setBitValue(DOMINANT);

            test_lib::TestSequence testSequence =
                test_lib::TestSequence(std::chrono::nanoseconds(10), driverFrame, monitorFrame);
            testSequence.pushDriverValuesToSimulator();
            testSequence.pushMonitorValuesToSimulator();

            canAgentMonitorSetTrigger(CAN_AGENT_MONITOR_TRIGGER_DRIVER_START);
            canAgentMonitorStart();

            canAgentDriverStart();
            canAgentDriverWaitFinish();

            canAgentCheckResult();

            testControllerAgentEndTest(true);

            testMessage("TestDemo: Run Exiting");

            return 0;

            /*****************************************************************
             * Test sequence end
             ****************************************************************/
        }
};