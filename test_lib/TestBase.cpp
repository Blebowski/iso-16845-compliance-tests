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
#include "../can_lib/can.h"

using namespace can;


test_lib::TestBase::TestBase()
{
    this->dut_ifc = new can::CtuCanFdInterface;
    this->dut_can_version = can::CanVersion::CanFdEnabled;
    this->test_result = true;
}

can::FrameType test_lib::TestBase::GetDefaultFrameType(TestVariant &variant)
{
    switch (variant){
        case TestVariant::Common:
            return FrameType::Can2_0;   /* Most of tests use CAN 2.0 for common */
        case TestVariant::Can_2_0:
            return FrameType::Can2_0;
        case TestVariant::CanFdTolerant: /* TODO: Check */
            return FrameType::Can2_0;
        case TestVariant::CanFdEnabled:
            return FrameType::CanFd;
        default:
            break;
    }
}

int test_lib::TestBase::Run()
{
    TestMessage("TestBase: Run Entered");    

    TestMessage("Querying test configuration from TB:");
    this->dut_clock_period = TestControllerAgentGetCfgDutClockPeriod();
    TestMessage("DUT clock period:");
    std::cout << this->dut_clock_period.count() << " ns" << std::endl;

    this->nominal_bit_timing.brp_ = TestControllerAgentGetBitTimingElement("CFG_DUT_BRP");
    this->nominal_bit_timing.prop_ = TestControllerAgentGetBitTimingElement("CFG_DUT_PROP");
    this->nominal_bit_timing.ph1_ = TestControllerAgentGetBitTimingElement("CFG_DUT_PH1");
    this->nominal_bit_timing.ph2_ = TestControllerAgentGetBitTimingElement("CFG_DUT_PH2");
    this->nominal_bit_timing.sjw_ = TestControllerAgentGetBitTimingElement("CFG_DUT_SJW");

    this->data_bit_timing.brp_ = TestControllerAgentGetBitTimingElement("CFG_DUT_BRP_FD");
    this->data_bit_timing.prop_ = TestControllerAgentGetBitTimingElement("CFG_DUT_PROP_FD");
    this->data_bit_timing.ph1_ = TestControllerAgentGetBitTimingElement("CFG_DUT_PH1_FD");
    this->data_bit_timing.ph2_ = TestControllerAgentGetBitTimingElement("CFG_DUT_PH2_FD");
    this->data_bit_timing.sjw_ = TestControllerAgentGetBitTimingElement("CFG_DUT_SJW_FD");

    this->seed = TestControllerAgentGetSeed();
    TestMessage("Seed: %d", this->seed);
    printf("Seed: %d\n", seed);
    srand(seed);

    TestMessage("Nominal Bit Timing configuration from TB:");
    this->nominal_bit_timing.Print();
    TestMessage("Data Bit Timing configuration from TB:");
    this->nominal_bit_timing.Print();
    
    TestMessage("Configuring Reset agent, executing reset");
    ResetAgentPolaritySet(0);
    ResetAgentAssert();
    ResetAgentDeassert();

    TestMessage("Configuring Clock generator agent");    
    ClockAgentSetPeriod(std::chrono::nanoseconds(this->dut_clock_period));
    ClockAgentSetJitter(std::chrono::nanoseconds(0));
    ClockAgentSetDuty(50);
    ClockAgentStart();

    TestMessage("Configuring Memory bus agent");
    MemBusAgentXModeStart();
    MemBusAgentSetPeriod(std::chrono::nanoseconds(this->dut_clock_period));
    memBusAgentSetXModeSetup(std::chrono::nanoseconds(2));
    MemBusAgentSetXModeHold(std::chrono::nanoseconds(2));
    MemBusAgentSetOutputDelay(std::chrono::nanoseconds(4));
    MemBusAgentStart();

    TestMessage("Configuring CAN Agent");
    CanAgentDriverFlush();
    CanAgentMonitorFlush();
    CanAgentDriverStop();
    CanAgentMonitorStop();
    CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(20));

    // Most of TCs use driver and monitor simultaneously, therefore there is no
    // need to configure Trigger in each of them!
    CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::DriverStart);

    TestMessage("Configuring DUT");
    this->dut_ifc->Reset();
    this->dut_ifc->ConfigureBitTiming(this->nominal_bit_timing, this->data_bit_timing);
    this->dut_ifc->ConfigureSsp(SspType::Disabled, 0);
    this->dut_ifc->SetCanVersion(this->dut_can_version);

    TestMessage("Enabling DUT");
    this->dut_ifc->Enable();

    TestMessage("Waiting till DUT is error active!");
    while (this->dut_ifc->GetErrorState() != FaultConfinementState::ErrorActive)
        usleep(2000);

    TestMessage("DUT ON! Test can start!");

    TestMessage("TestBase: Run Exiting");

    return 0;
}

void test_lib::TestBase::ConfigureTest()
{
}

void test_lib::TestBase::SetupTestEnvironment()
{
    TestBigMessage("Running base test...");
    TestBase::Run();
    TestMessage("Done");

    TestBigMessage("Running test specific config...");
    ConfigureTest();
    TestMessage("Done");

    PrintTestInfo();

    TestBigMessage("Starting test execution: ", test_name);
}


test_lib::TestResult test_lib::TestBase::FinishTest()
{
    TestBigMessage("Cleaning up test environemnt...");
    TestControllerAgentEndTest((int)test_result);
    TestBigMessage("Finishing test execution: ", test_name);
    return (TestResult) test_result;
}


test_lib::TestResult test_lib::TestBase::FinishTest(TestResult test_result)
{
    this->test_result = (int) test_result;
    TestBigMessage("Cleaning up test environemnt...");
    TestControllerAgentEndTest((int)test_result);
    TestBigMessage("Finishing test execution: ", test_name);
    return (TestResult) test_result;
}

void test_lib::TestBase::FillTestVariants(VariantMatchingType match_type)
{
    switch (match_type)
    {
    case VariantMatchingType::OneToOne:
        switch (dut_can_version)
        {
        case CanVersion::Can_2_0:
            test_variants.push_back(TestVariant::Can_2_0);
            break;
        case CanVersion::CanFdTolerant:
            test_variants.push_back(TestVariant::CanFdTolerant);
            break;
        case CanVersion::CanFdEnabled:
            test_variants.push_back(TestVariant::CanFdEnabled);
            break;
        default:
            break;
        }
        elem_tests.push_back(std::vector<ElementaryTest>());
        break;

    case VariantMatchingType::Common:
        test_variants.push_back(TestVariant::Common);
        elem_tests.push_back(std::vector<ElementaryTest>());
        break;

    case VariantMatchingType::CommonAndFd:
        test_variants.push_back(TestVariant::Common);
        elem_tests.push_back(std::vector<ElementaryTest>());

        if (dut_can_version == CanVersion::CanFdEnabled)
        {
            test_variants.push_back(TestVariant::CanFdEnabled);
            elem_tests.push_back(std::vector<ElementaryTest>());
        }

    default:
        break;
    }
}


std::unique_ptr<BitFrame> test_lib::TestBase::ConvertBitFrame(Frame &golden_frame)
{
    return std::make_unique<BitFrame>(
        golden_frame, &nominal_bit_timing, &data_bit_timing);
}


/**
 * Note that operator overloading was not used on purpose because if operator is
 * overloaded it is non-member function of class. When this is linked with GHDL
 * simulation, it throws out linkage errors!
 */
bool test_lib::TestBase::CompareFrames(can::Frame &expected_frame, can::Frame &real_frame)
{
    bool retVal = true;

    if (expected_frame.identifier() != real_frame.identifier())
        retVal = false;
    if (expected_frame.dlc() != real_frame.dlc())
        retVal = false;

    // Compare frame flags
    if (!(expected_frame.frame_flags().is_brs_ == real_frame.frame_flags().is_brs_))
        retVal = false;
    if (!(expected_frame.frame_flags().is_esi_ == real_frame.frame_flags().is_esi_))
        retVal = false;
    if (!(expected_frame.frame_flags().is_fdf_ == real_frame.frame_flags().is_fdf_))
        retVal = false;
    if (!(expected_frame.frame_flags().is_ide_ == real_frame.frame_flags().is_ide_))
        retVal = false;
    if (!(expected_frame.frame_flags().is_rtr_ == real_frame.frame_flags().is_rtr_))
        retVal = false;

    for (int i = 0; i < expected_frame.data_length(); i++)
        if (expected_frame.data(i) != real_frame.data(i))
            retVal = false;

    if (retVal == false)
    {
        TestMessage("Frame read from DUT does not match send frame!");
        TestMessage("Expected frame:");
        expected_frame.Print();
        TestMessage("Real frame:");
        real_frame.Print();
    }
    return retVal;
}


void test_lib::TestBase::CheckRxFrame(Frame &golden_frame)
{
    // Read received frame from DUT and compare with sent frame
    Frame read_frame = dut_ifc->ReadFrame();
    if (CompareFrames(golden_frame, read_frame) == false)
    {
        test_result = (int) TestResult::Failed;
        TestControllerAgentEndTest(false);
    }
}


void test_lib::TestBase::CheckNoRxFrame()
{
    if (dut_ifc->HasRxFrame())
    {
        TestMessage("DUT has received frame but it shouldnt!");
        test_result = false;
    }
}


void test_lib::TestBase::CheckRecChange(int reference_rec, int delta)
{
    int rec_new = dut_ifc->GetRec();
    if (rec_new != (reference_rec + delta))
    {
        TestMessage("DUT REC not as expected. Expected %d, Real %d",
                        rec_old + delta, rec_new);
        test_result = false;
    }
}


void test_lib::TestBase::CheckTecChange(int reference_tec, int delta)
{
    int tec_new = dut_ifc->GetRec();
    if (tec_new != (reference_tec + delta))
    {
        TestMessage("DUT TEC not as expected. Expected %d, Real %d",
                        rec_old + delta, rec_new);
        test_result = false;
    }
}


void test_lib::TestBase::PushFramesToLowerTester(can::BitFrame &driver_bit_frame,
                                                 can::BitFrame &monitor_bit_frame)
{
    TestSequence *test_sequence;

    test_sequence = new TestSequence(this->dut_clock_period, driver_bit_frame, monitor_bit_frame);
    test_sequence->PushDriverValuesToSimulator();
    test_sequence->PushMonitorValuesToSimulator();

    delete test_sequence;
}


void test_lib::TestBase::RunLowerTester(bool start_driver, bool start_monitor)
{

    // Note: It is important to start monitor first because it waits for driver
    //       in most cases!

    if (start_monitor)
        CanAgentMonitorStart();
    if (start_driver)
        CanAgentDriverStart();

    if (start_monitor)
        CanAgentMonitorWaitFinish();
    if (start_driver)
        CanAgentDriverWaitFinish();

    TestMessage("Lower tester (CAN agent) ended!");
}


void test_lib::TestBase::StartDriverAndMonitor()
{
    CanAgentMonitorStart();
    CanAgentDriverStart();
}


void test_lib::TestBase::WaitForDriverAndMonitor()
{
    CanAgentMonitorWaitFinish();
    CanAgentDriverWaitFinish();
}


void test_lib::TestBase::CheckLowerTesterResult()
{
    CanAgentCheckResult();
    CanAgentMonitorStop();
    CanAgentDriverStop();
    CanAgentMonitorFlush();
    CanAgentDriverFlush();
}

void test_lib::TestBase::PrintTestInfo()
{
    TestMessage(std::string(80, '*'));
    TestMessage("Test Name: ", test_name);
    TestMessage("Number of variants: ", elem_tests.size());
    TestMessage("Number of elementary tests (per-variant):", num_elem_tests);
}

void test_lib::TestBase::PrintElemTestInfo(ElementaryTest elem_test)
{
    TestBigMessage("Elementary Test: ", elem_test.msg);
}

void test_lib::TestBase::PrintVariantInfo(TestVariant test_variant)
{
    switch (test_variant)
    {
        case TestVariant::Can_2_0:
            TestBigMessage("Test variant: CAN 2.0!");
            break;
        case TestVariant::CanFdEnabled:
            TestBigMessage("Test variant: CAN FD Enabled!");
            break;
        case TestVariant::CanFdTolerant:
            TestBigMessage("Test variant: CAN FD Tolerant");
            break;
        case TestVariant::Common:
            TestBigMessage("Test variant: Common");
            break;
        default:
            assert(("Unsupported variant!", false));
            break;
        }
}

void test_lib::TestBase::RandomizeAndPrint(Frame *frame)
{
    frame->Randomize();
    TestMessage("Test frame:");
    frame->Print();
}

void test_lib::TestBase::DeleteCommonObjects()
{
    delete golden_frame;
    delete driver_bit_frame;
    delete monitor_bit_frame;
}
