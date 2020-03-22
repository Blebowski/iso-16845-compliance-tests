/**
 * TODO: License
 */

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "../vpi_lib/vpiComplianceLib.hpp"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"

#include "../compliance_tests_includes/TestDemo.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;

test_lib::TestDemo::TestDemo() : TestBase()
{
    //TODO
};


int test_lib::TestDemo::run()
{
    TestBase::run();

    testMessage("TestDemo: Run Entered");
    int polarity = 1;
    int polarity_read = 0;

    std::chrono::nanoseconds clkPer(10);    
    memBusAgentSetPeriod(clkPer);
    memBusAgentStart();
    memBusAgentWrite32(16, 0xAABBCCDD);
    uint8_t A;
    A = memBusAgentRead8(0);
    A = memBusAgentRead8(1);
    A = memBusAgentRead8(2);
    A = memBusAgentRead8(3);
    
    uint16_t B = memBusAgentRead16(0);
    testMessage("H");
    
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

    BitTiming nbt = BitTiming(2, 2, 2, 4, 1);
    BitTiming dbt = BitTiming(2, 2, 2, 1, 1);

    BitFrame bitFrame = BitFrame(
        FrameFlags(CAN_FD, EXTENDED_IDENTIFIER, DATA_FRAME,
                   BIT_RATE_DONT_SHIFT, ESI_ERROR_ACTIVE),
        0, 32, &(data[0]), &nbt, &dbt);

    bitFrame.print(true);

    test_lib::TestSequence testSequence =
        test_lib::TestSequence(std::chrono::nanoseconds(10), bitFrame, test_lib::DRIVER_SEQUENCE);
    testSequence.pushDriverValuesToSimulator();

    canAgentDriverStart();
    canAgentDriverWaitFinish();

    testControllerAgentEndTest(false);

    testMessage("TestDemo: Run Exiting");

    return 0;
}