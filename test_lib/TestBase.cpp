/**
 * TODO: License
 */

#include <iostream>

#include "test_lib.h"
#include "TestBase.h"
#include "TestLoader.h"
#include <unistd.h>

#include "../test_lib/test_lib.h"
#include "../vpi_lib/vpiComplianceLib.hpp"
#include "../can_lib/CtuCanFdInterface.h"

test_lib::TestBase::TestBase()
{
    return;
}


int test_lib::TestBase::run()
{
    testMessage("TestBase: Run Entered");

    // Create DUT interface!
    this->dutIfc = new can::CtuCanFdInterface;

    testMessage("Querying test configuration from TB:");
    this->dutClockPeriod = testControllerAgentGetCfgDutClockPeriod();
    testMessage("DUT clock period:");
    std::cout << this->dutClockPeriod.count() << " ns" << std::endl;

    this->nominalBitTiming.brp = testControllerAgentGetBitTimingElement("CFG_DUT_BRP");
    this->nominalBitTiming.prop = testControllerAgentGetBitTimingElement("CFG_DUT_PROP");
    this->nominalBitTiming.ph1 = testControllerAgentGetBitTimingElement("CFG_DUT_PH1");
    this->nominalBitTiming.ph2 = testControllerAgentGetBitTimingElement("CFG_DUT_PH2");
    this->nominalBitTiming.sjw = testControllerAgentGetBitTimingElement("CFG_DUT_SJW");

    this->dataBitTiming.brp = testControllerAgentGetBitTimingElement("CFG_DUT_BRP_FD");
    this->dataBitTiming.prop = testControllerAgentGetBitTimingElement("CFG_DUT_PROP_FD");
    this->dataBitTiming.ph1 = testControllerAgentGetBitTimingElement("CFG_DUT_PH1_FD");
    this->dataBitTiming.ph2 = testControllerAgentGetBitTimingElement("CFG_DUT_PH2_FD");
    this->dataBitTiming.sjw = testControllerAgentGetBitTimingElement("CFG_DUT_SJW_FD");

    testMessage("Nominal Bit Timing configuration from TB:");
    this->nominalBitTiming.print();
    testMessage("Data Bit Timing configuration from TB:");
    this->nominalBitTiming.print();
    
    testMessage("Configuring Reset agent, executing reset");
    resetAgentPolaritySet(0);
    resetAgentAssert();
    resetAgentDeassert();

    testMessage("Configuring Clock generator agent");    
    clockAgentSetPeriod(std::chrono::nanoseconds(this->dutClockPeriod));
    clockAgentSetJitter(std::chrono::nanoseconds(0));
    clockAgentSetDuty(50);
    clockAgentStart();

    testMessage("Configuring Memory bus agent");
    memBusAgentXModeStart();
    memBusAgentSetPeriod(std::chrono::nanoseconds(this->dutClockPeriod));
    memBusAgentSetXModeSetup(std::chrono::nanoseconds(2));
    memBusAgentSetXModeHold(std::chrono::nanoseconds(2));
    memBusAgentSetOutputDelay(std::chrono::nanoseconds(4));
    memBusAgentStart();

    testMessage("Configuring CAN Agent");
    canAgentDriverFlush();
    canAgentMonitorFlush();
    canAgentDriverStop();
    canAgentMonitorStop();
    canAgentSetMonitorInputDelay(std::chrono::nanoseconds(10));
    canAgentMonitorSetSampleRate(std::chrono::nanoseconds(this->dutClockPeriod));

    testMessage("Configuring DUT");
    this->dutIfc->reset();
    this->dutIfc->configureBitTiming(this->nominalBitTiming, this->dataBitTiming);
    
    testMessage("Enabling DUT");
    this->dutIfc->enable();

    testMessage("Waiting till DUT is error active!");
    while (this->dutIfc->getErrorState() != can::ERROR_ACTIVE)
        usleep(2000);

    testMessage("DUT ON! Test can start!");

    testMessage("TestBase: Run Exiting");
    return 0;
}