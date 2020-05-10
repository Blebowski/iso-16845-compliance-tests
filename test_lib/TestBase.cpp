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

#include "test_lib.h"
#include "TestBase.h"
#include "TestLoader.h"
#include "TestSequence.h"
#include <unistd.h>

#include "../test_lib/test_lib.h"
#include "../vpi_lib/vpiComplianceLib.hpp"
#include "../can_lib/CtuCanFdInterface.h"


test_lib::TestBase::TestBase()
{
    this->dutIfc = new can::CtuCanFdInterface;
    this->canVersion = can::CanVersion::CAN_FD_ENABLED_VERSION;
    this->testResult = true;
}


int test_lib::TestBase::run()
{
    testMessage("TestBase: Run Entered");    

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

    this->seed = testControllerAgentGetSeed();
    testMessage("Seed: %d", this->seed);
    printf("Seed: %d\n", seed);
    srand(seed);

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
    canAgentSetMonitorInputDelay(std::chrono::nanoseconds(20));

    // Most of TCs use driver and monitor simultaneously, therefore there is no
    // need to configure Trigger in each of them!
    canAgentMonitorSetTrigger(CAN_AGENT_MONITOR_TRIGGER_DRIVER_START);

    testMessage("Configuring DUT");
    this->dutIfc->reset();
    this->dutIfc->configureBitTiming(this->nominalBitTiming, this->dataBitTiming);
    this->dutIfc->setCanVersion(this->canVersion);

    testMessage("Enabling DUT");
    this->dutIfc->enable();

    testMessage("Waiting till DUT is error active!");
    while (this->dutIfc->getErrorState() != can::ERROR_ACTIVE)
        usleep(2000);

    testMessage("DUT ON! Test can start!");

    testMessage("TestBase: Run Exiting");

    return 0;
}


/**
 * Note that operator overloading was not used on purpose because if operator is
 * overloaded it is non-member function of class. When this is linked with GHDL
 * simulation, it throws out linkage errors!
 */
bool test_lib::TestBase::compareFrames(can::Frame &expectedFrame, can::Frame &realFrame)
{
    bool retVal = true;

    if (expectedFrame.getIdentifier() != realFrame.getIdentifier())
        retVal = false;
    if (expectedFrame.getDlc() != realFrame.getDlc())
        retVal = false;

    // Compare frame flags
    if (!(expectedFrame.getFrameFlags().isBrs_ == realFrame.getFrameFlags().isBrs_))
        retVal = false;
    if (!(expectedFrame.getFrameFlags().isEsi_ == realFrame.getFrameFlags().isEsi_))
        retVal = false;
    if (!(expectedFrame.getFrameFlags().isFdf_ == realFrame.getFrameFlags().isFdf_))
        retVal = false;
    if (!(expectedFrame.getFrameFlags().isIde_ == realFrame.getFrameFlags().isIde_))
        retVal = false;
    if (!(expectedFrame.getFrameFlags().isRtr_ == realFrame.getFrameFlags().isRtr_))
        retVal = false;

    for (int i = 0; i < expectedFrame.getDataLenght(); i++)
        if (expectedFrame.getData(i) != realFrame.getData(i))
            retVal = false;

    if (retVal == false)
    {
        testMessage("Frame read from DUT does not match send frame!");
        testMessage("Expected frame:");
        expectedFrame.print();
        testMessage("Real frame:");
        realFrame.print();
    }
    return retVal;
}


void test_lib::TestBase::pushFramesToLowerTester(can::BitFrame &driverBitFrame,
                                                 can::BitFrame &monitorBitFrame)
{
    TestSequence *testSequence;

    testSequence = new TestSequence(this->dutClockPeriod, driverBitFrame, monitorBitFrame);
    testSequence->pushDriverValuesToSimulator();
    testSequence->pushMonitorValuesToSimulator();

    delete testSequence;
}


void test_lib::TestBase::runLowerTester(bool startDriver, bool startMonitor)
{

    // Note: It is important to start monitor first because it waits for driver
    //       in most cases!

    if (startMonitor)
        canAgentMonitorStart();
    if (startDriver)
        canAgentDriverStart();

    if (startDriver)
        canAgentDriverWaitFinish();
    if (startMonitor)
        canAgentMonitorWaitFinish();

    testMessage("Lower tester (CAN agent) ended!");
}


void test_lib::TestBase::checkLowerTesterResult()
{
    canAgentCheckResult();
    canAgentMonitorStop();
    canAgentDriverStop();
    canAgentMonitorFlush();
    canAgentDriverFlush();
}

void test_lib::TestBase::deleteCommonObjects()
{
    delete goldenFrame;
    delete driverBitFrame;
    delete monitorBitFrame;
}
