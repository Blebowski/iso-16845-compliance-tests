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

test_lib::TestBase::~TestBase()
{
    delete this->dut_ifc;
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
    return FrameType::Can2_0;
}


void test_lib::TestBase::ConfigureTest()
{
    TestMessage("TestBase: Configuration Entered");    

    TestMessage("Querying test configuration from TB:");
    this->dut_clock_period = TestControllerAgentGetCfgDutClockPeriod();
    TestMessage("DUT clock period:");
    std::cout << this->dut_clock_period.count() << " ns" << std::endl;

    // TODO: Query input delay from TB, and eventually from VIP configuration !!!
    this->dut_input_delay = 2;
    TestMessage("DUT input delay:");
    std::cout << "2 clock cycles" << std::endl;

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
    this->data_bit_timing.Print();
    
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

    TestMessage("TestBase: Configuration Exiting");
}


void test_lib::TestBase::SetupTestEnvironment()
{
    TestBigMessage("Base test config...");
    TestBase::ConfigureTest();
    TestMessage("Done");

    TestBigMessage("Test specific config...");
    ConfigureTest();
    TestMessage("Done");

    PrintTestInfo();

    TestBigMessage("Starting test execution: ", test_name);
}


void test_lib::TestBase::SetupMonitorTxTests()
{
    CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
    CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
    CanAgentSetWaitForMonitor(true);
}


int test_lib::TestBase::Run()
{
    SetupTestEnvironment();

    int variant_index = 0;

    /*
    if (RunElemTest == 0)
    {
        TestBigMessage("Elementary test Run routine not defined, exiting...");
        test_result = false;
        return (int)FinishTest();
    }
    */

    for (auto const &test_variant : test_variants)
    {
        PrintVariantInfo(test_variant);

        /* Used only in few tests with more stuff bits in single variant! */
        stuff_bits_in_variant = 0;

        for (auto const & elem_test : elem_tests[variant_index])
        {
            PrintElemTestInfo(elem_test);

            if (RunElemTest(elem_test, test_variant) != 0)
            {
                TestBigMessage("Elementary test %d failed.", elem_test.index);
                return (int)FinishTest();
            }
        }

        if (stuff_bits_in_variant > 0)
            TestMessage("FINAL number of stuff bits in variant: %d", stuff_bits_in_variant);

        variant_index++;
    }
    return (int)FinishTest();
}


int test_lib::TestBase::FinishElementaryTest()
{
    if (test_result)
        return 0;
    return 1;
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
        break;

    case VariantMatchingType::ClassicalAndFdEnabled:
        if (dut_can_version == CanVersion::Can_2_0)
            test_variants.push_back(TestVariant::Can_2_0);
        if (dut_can_version == CanVersion::CanFdEnabled)
            test_variants.push_back(TestVariant::CanFdEnabled);
        elem_tests.push_back(std::vector<ElementaryTest>());
        break;

    case VariantMatchingType::FdTolerantFdEnabled:
        if (dut_can_version == CanVersion::CanFdTolerant)
            test_variants.push_back(TestVariant::CanFdTolerant);
        if (dut_can_version == CanVersion::CanFdEnabled)
            test_variants.push_back(TestVariant::CanFdEnabled);
        elem_tests.push_back(std::vector<ElementaryTest>());
        break;
    
    case VariantMatchingType::ClassicalFdCommon:
        if (dut_can_version == CanVersion::Can_2_0)
            test_variants.push_back(TestVariant::Can_2_0);
        if (dut_can_version == CanVersion::CanFdTolerant)
            test_variants.push_back(TestVariant::CanFdTolerant);
        elem_tests.push_back(std::vector<ElementaryTest>());
        if (dut_can_version == CanVersion::CanFdEnabled)
        {
            test_variants.push_back(TestVariant::CanFdTolerant);
            test_variants.push_back(TestVariant::CanFdEnabled);
            elem_tests.push_back(std::vector<ElementaryTest>());
        }
        break;

    case VariantMatchingType::CanFdEnabledOnly:
        if (dut_can_version == CanVersion::CanFdEnabled)
        {
            test_variants.push_back(TestVariant::CanFdEnabled);
            elem_tests.push_back(std::vector<ElementaryTest>());
        }

    default:
        break;
    }
}


void test_lib::TestBase::AddElemTest(TestVariant test_variant, ElementaryTest &&elem_test)
{
    int i = 0;
    for (auto &test_variant_it : test_variants)
    {
        if (test_variant_it == test_variant)
        {
            elem_tests[i].push_back(elem_test);
            return;
        }
        ++i;
    }
    TestMessage("Test variant not found! Ignoring elementary test.");
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


BitType test_lib::TestBase::GetRandomBitType(FrameType frame_type, IdentifierType ident_type,
                                            BitField bit_field)
{
    switch (bit_field)
    {
    case BitField::Sof:
        return BitType::Sof;

    case BitField::Arbitration:
        if (ident_type == IdentifierType::Base)
        {
            if (rand() % 2)
                return BitType::BaseIdentifier;
            if (frame_type == FrameType::Can2_0)
                return BitType::Rtr;
            return BitType::R1;

        } else {
            switch (rand() % 5)
            {
            case 0:
                return BitType::BaseIdentifier;
            case 1:
                return BitType::Srr;
            case 2:
                return BitType::Ide;
            case 3:
                return BitType::IdentifierExtension;
            default:
                if (frame_type == FrameType::Can2_0)
                    return BitType::Rtr;
                return BitType::R1;
            }
        }
        return BitType::BaseIdentifier;

    case BitField::Control:
        if (frame_type == FrameType::Can2_0)
        {
            switch (rand() % 3)
            {
            case 0:
                if (ident_type == IdentifierType::Base)
                    return BitType::Ide;
                return BitType::R1;
            case 1:
                return BitType::R0;
            default:
                return BitType::Dlc;
            }

        } else {
            switch (rand() % 5)
            {
            case 0:
                return BitType::Edl;
            case 1:
                return BitType::R0;
            case 2:
                return BitType::Brs;
            case 3:
                return BitType::Esi;
            default:
                return BitType::Dlc;
            }
        }
    
    case BitField::Data:
        return BitType::Data;

    case BitField::Crc:
        if (frame_type == FrameType::CanFd)
        {
            switch (rand() % 3)
            {
            case 0:
                return BitType::StuffCount;
            case 1:
                return BitType::StuffParity;
            default:
                return BitType::Crc;
            }
        } else {
            return BitType::Crc;
        }

    case BitField::Ack:
        if (rand() % 2)
            return BitType::CrcDelimiter;
        return BitType::AckDelimiter;

    case BitField::Eof:
        return BitType::Eof;
    }
    return BitType::BaseIdentifier;
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
    } else {
        TestMessage("DUT REC change as expected! Expected %d, Real %d",
                        rec_old + delta, rec_new);
    }
}


void test_lib::TestBase::CheckTecChange(int reference_tec, int delta)
{
    int tec_new = dut_ifc->GetTec();
    if (tec_new != (reference_tec + delta))
    {
        TestMessage("DUT TEC change NOT as expected. Expected %d, Real %d",
                        tec_old + delta, tec_new);
        test_result = false;
    } else {
        TestMessage("DUT TEC change as expected! Expected %d, Real %d",
                        tec_old + delta, tec_new);
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

int test_lib::TestBase::RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                                    [[maybe_unused]] const TestVariant &test_variant)
{
    return 0;
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
    TestMessage(std::string(80, '*').c_str());
    TestMessage("Test Name: %s", test_name.c_str());
    TestMessage("Number of variants: %d", test_variants.size());
    int num_elem_tests = 0;
    for (const auto &variant_tests : elem_tests)
        num_elem_tests += variant_tests.size();
    TestMessage("Total number of elementary tests: %d", num_elem_tests);
}

void test_lib::TestBase::PrintElemTestInfo(ElementaryTest elem_test)
{
    TestMessage(std::string(80, '*').c_str());
    TestMessage("Elementary Test index: %d", elem_test.index);
    //TestMessage("Elementary Test message: %s", elem_test.msg.c_str());
    TestMessage(std::string(80, '*').c_str());
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
            assert(false && "Unsupported variant!");
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

void test_lib::TestBase::FreeTestObjects()
{
    golden_frm.reset();
    golden_frm_2.reset();
    driver_bit_frm.reset();
    driver_bit_frm_2.reset();
    monitor_bit_frm.reset();
    monitor_bit_frm_2.reset();
}
