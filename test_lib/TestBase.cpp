/**
 * TODO: License
 */

#include <iostream>

#include "test_lib.h"
#include "TestBase.h"
#include "TestLoader.h"
#include <unistd.h>

#include "../test_lib/test_lib.h"
#include "../vpi_lib/vpi_compliance_lib.hpp"

test_lib::TestBase::TestBase()
{
    return;
}

int test_lib::TestBase::run()
{
    testMessage("TestBase: Run Entered");

    testMessage("Configuring Reset agent, executing reset");
    resetAgentPolaritySet(0);
    resetAgentAssert();
    resetAgentDeassert();

    testMessage("Configuring Clock generator agent");    
    clockAgentSetPeriod(std::chrono::nanoseconds(10)); // TODO: Use clock period provided by configuration from VUnit!
    clockAgentSetJitter(std::chrono::nanoseconds(0));
    clockAgentSetDuty(50);

    testMessage("Configuring Memory bus agent");
    memBusAgentXModeStart();
    memBusAgentSetPeriod(std::chrono::nanoseconds(10)); // TODO: Use clock period provided by configuration from VUnit!
    memBusAgentSetXModeSetup(std::chrono::nanoseconds(2));
    memBusAgentSetXModeHold(std::chrono::nanoseconds(2));
    memBusAgentSetOutputDelay(std::chrono::nanoseconds(4));
    memBusAgentStart();

    testMessage("Configuring CAN Agent");
    canAgentDriverFlush();
    canAgentMonitorFlush();
    canAgentDriverStop();
    canAgentMonitorStop();
    canAgentMonitorSetSampleRate(std::chrono::nanoseconds(1));

    testMessage("TestBase: Run Exiting");
    return 0;
}