/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>

#include <pli_lib.h>

#include "TestBase.h"

using namespace can;


test::TestBase::TestBase()
{
    this->dut_ifc = new can::CtuCanFdInterface;
    this->dut_can_version = can::CanVersion::CanFdEna;
    this->test_result = true;
}

test::TestBase::~TestBase()
{
    delete this->dut_ifc;
}

can::FrameKind test::TestBase::GetDefaultFrameType(TestVariant &variant)
{
    switch (variant){
        case TestVariant::Common:
            return FrameKind::Can20;   /* Most of tests use CAN 2.0 for common */
        case TestVariant::Can_2_0:
            return FrameKind::Can20;
        case TestVariant::CanFdTolerant: /* TODO: Check */
            return FrameKind::Can20;
        case TestVariant::CanFdEnabled:
            return FrameKind::CanFd;
        default:
            break;
    }
    return FrameKind::Can20;
}


void test::TestBase::ConfigureTest()
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

    // TODO: Query DUTs information processing time from TB!
    this->dut_ipt = 2;

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

    // Create backup, so that we can change the actual bit-timing by test.
    backup_nominal_bit_timing = nominal_bit_timing;
    backup_data_bit_timing = data_bit_timing;

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

    // Default Monitor delay (used for RX tests), must correspond to IUTs input delay!
    // Then if driver starts at time T, monitor will start at proper time t + x, where
    // x corresponds to input delay. Due to this, monitor will be in sync with IUT exactly!
    CanAgentSetMonitorInputDelay(dut_input_delay * dut_clock_period);

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

    WaitDutErrorActive();

    TestMessage("DUT ON! Test can start!");
    TestMessage("TestBase: Configuration Exiting");
}


void test::TestBase::SetupTestEnvironment()
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


void test::TestBase::SetupMonitorTxTests()
{
    CanAgentMonitorSetTrigger(CanAgentMonitorTrigger::TxFalling);
    CanAgentSetMonitorInputDelay(std::chrono::nanoseconds(0));
    CanAgentSetWaitForMonitor(true);
}


int test::TestBase::Run()
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
                TestBigMessage("Elementary test %d failed.", elem_test.index_);
                return (int)FinishTest();
            }
        }

        if (stuff_bits_in_variant > 0)
            TestMessage("FINAL number of stuff bits in variant: %d", stuff_bits_in_variant);

        variant_index++;
    }
    return (int)FinishTest();
}


int test::TestBase::FinishElementaryTest()
{
    if (test_result)
        return 0;
    return 1;
}


test::TestResult test::TestBase::FinishTest()
{
    TestBigMessage("Cleaning up test environemnt...");
    TestControllerAgentEndTest((int)test_result);
    TestBigMessage("Finishing test execution: ", test_name);
    return (TestResult) test_result;
}


test::TestResult test::TestBase::FinishTest(TestResult test_result)
{
    this->test_result = (int) test_result;
    TestBigMessage("Cleaning up test environemnt...");
    TestControllerAgentEndTest((int)test_result);
    TestBigMessage("Finishing test execution: ", test_name);
    return (TestResult) test_result;
}

void test::TestBase::FillTestVariants(VariantMatchingType match_type)
{
    switch (match_type)
    {
    case VariantMatchingType::OneToOne:
        switch (dut_can_version)
        {
        case CanVersion::Can20:
            test_variants.push_back(TestVariant::Can_2_0);
            break;
        case CanVersion::CanFdTol:
            test_variants.push_back(TestVariant::CanFdTolerant);
            break;
        case CanVersion::CanFdEna:
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

        if (dut_can_version == CanVersion::CanFdEna)
        {
            test_variants.push_back(TestVariant::CanFdEnabled);
            elem_tests.push_back(std::vector<ElementaryTest>());
        }
        break;

    case VariantMatchingType::ClassicalAndFdEnabled:
        if (dut_can_version == CanVersion::Can20)
            test_variants.push_back(TestVariant::Can_2_0);
        if (dut_can_version == CanVersion::CanFdEna)
            test_variants.push_back(TestVariant::CanFdEnabled);
        elem_tests.push_back(std::vector<ElementaryTest>());
        break;

    case VariantMatchingType::FdTolerantFdEnabled:
        if (dut_can_version == CanVersion::CanFdTol)
            test_variants.push_back(TestVariant::CanFdTolerant);
        if (dut_can_version == CanVersion::CanFdEna)
            test_variants.push_back(TestVariant::CanFdEnabled);
        elem_tests.push_back(std::vector<ElementaryTest>());
        break;

    case VariantMatchingType::ClassicalFdCommon:
        if (dut_can_version == CanVersion::Can20)
            test_variants.push_back(TestVariant::Can_2_0);
        if (dut_can_version == CanVersion::CanFdTol)
            test_variants.push_back(TestVariant::CanFdTolerant);
        elem_tests.push_back(std::vector<ElementaryTest>());
        if (dut_can_version == CanVersion::CanFdEna)
        {
            test_variants.push_back(TestVariant::CanFdTolerant);
            test_variants.push_back(TestVariant::CanFdEnabled);
            elem_tests.push_back(std::vector<ElementaryTest>());
        }
        break;

    case VariantMatchingType::CanFdEnabledOnly:
        if (dut_can_version == CanVersion::CanFdEna)
        {
            test_variants.push_back(TestVariant::CanFdEnabled);
            elem_tests.push_back(std::vector<ElementaryTest>());
        }

    default:
        break;
    }
}


void test::TestBase::AddElemTest(TestVariant test_variant, ElementaryTest &&elem_test)
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


void test::TestBase::AddElemTestForEachSamplePoint(TestVariant test_variant,
                            bool nominal, FrameKind frame_type)
{
    TestMessage("Adding Elementary tests for each sample point...");

    int num_sp_points = CalcNumSamplePoints(nominal);

    TestMessage("Number of sample points: %d", num_sp_points);

    for (int i = 1; i <= num_sp_points; i++)
        AddElemTest(test_variant, ElementaryTest(i, frame_type));
}


size_t test::TestBase::GetDefaultMinPh1(BitTiming *orig_bt, bool nominal)
{
    // Respect CTU CAN FDs minimal TSEG1 duration in clock cycles:
    //      Nominal = 5
    //      Data = 3
    //
    // TODO: Make minimal durations configurable from TB, not hardcoded for CTU CAN FD!
    if (nominal)
    {
        if (orig_bt->brp_ == 1) {
            return 4;
        } else if (orig_bt->brp_ == 2) {
            return 2;
        } else if (orig_bt->brp_ == 3) {
            return 1;
        } else if (orig_bt->brp_ == 4) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (orig_bt->brp_ == 1) {
            return 2;
        } else if (orig_bt->brp_ == 2) {
            return 1;
        } else {
            return 0;
        }
    }
}


BitTiming test::TestBase::GenerateSamplePointForTest(const ElementaryTest &elem_test, bool nominal)
{
    return GenerateBitTiming(elem_test, nominal, 0);
}


BitTiming test::TestBase::GenerateSamplePointForTest(const ElementaryTest &elem_test, bool nominal,
                                                         size_t minimal_ph1)
{
    return GenerateBitTiming(elem_test, nominal, minimal_ph1);
}


std::unique_ptr<BitFrame> test::TestBase::ConvertBitFrame(Frame &golden_frame)
{
    return std::make_unique<BitFrame>(
        golden_frame, &nominal_bit_timing, &data_bit_timing);
}


/**
 * Note that operator overloading was not used on purpose because if operator is
 * overloaded it is non-member function of class. When this is linked with GHDL
 * simulation, it throws out linkage errors!
 */
bool test::TestBase::CompareFrames(can::Frame &expected_frame, can::Frame &real_frame)
{
    bool ret_val = true;

    if (expected_frame.identifier() != real_frame.identifier())
        ret_val = false;
    if (expected_frame.dlc() != real_frame.dlc())
        ret_val = false;
    if (expected_frame.frame_flags() != real_frame.frame_flags())
        ret_val = false;

    for (int i = 0; i < expected_frame.data_length(); i++)
        if (expected_frame.data(i) != real_frame.data(i))
            ret_val = false;

    if (!ret_val)
    {
        TestMessage("Frame read from DUT does not match send frame!");
        TestMessage("Expected frame:");
        expected_frame.Print();
        TestMessage("Real frame:");
        real_frame.Print();
    }
    return ret_val;
}


BitKind test::TestBase::GetRandomBitType(FrameKind frame_type, IdentKind ident_type,
                                            BitField bit_field)
{
    switch (bit_field)
    {
    case BitField::Sof:
        return BitKind::Sof;

    case BitField::Arbit:
        if (ident_type == IdentKind::Base)
        {
            if (rand() % 2)
                return BitKind::BaseIdent;
            if (frame_type == FrameKind::Can20)
                return BitKind::Rtr;
            return BitKind::R1;

        } else {
            switch (rand() % 5)
            {
            case 0:
                return BitKind::BaseIdent;
            case 1:
                return BitKind::Srr;
            case 2:
                return BitKind::Ide;
            case 3:
                return BitKind::ExtIdent;
            default:
                if (frame_type == FrameKind::Can20)
                    return BitKind::Rtr;
                return BitKind::R1;
            }
        }
        return BitKind::BaseIdent;

    case BitField::Control:
        if (frame_type == FrameKind::Can20)
        {
            switch (rand() % 3)
            {
            case 0:
                if (ident_type == IdentKind::Base)
                    return BitKind::Ide;
                return BitKind::R1;
            case 1:
                return BitKind::R0;
            default:
                return BitKind::Dlc;
            }

        } else {
            switch (rand() % 5)
            {
            case 0:
                return BitKind::Edl;
            case 1:
                return BitKind::R0;
            case 2:
                return BitKind::Brs;
            case 3:
                return BitKind::Esi;
            default:
                return BitKind::Dlc;
            }
        }

    case BitField::Data:
        return BitKind::Data;

    case BitField::Crc:
        if (frame_type == FrameKind::CanFd)
        {
            switch (rand() % 3)
            {
            case 0:
                return BitKind::StuffCnt;
            case 1:
                return BitKind::StuffParity;
            default:
                return BitKind::Crc;
            }
        } else {
            return BitKind::Crc;
        }

    case BitField::Ack:
        if (rand() % 2)
            return BitKind::CrcDelim;
        return BitKind::AckDelim;

    case BitField::Eof:
        return BitKind::Eof;
    }
    return BitKind::BaseIdent;
}

void test::TestBase::CheckRxFrame(Frame &golden_frame)
{
    // Read received frame from DUT and compare with sent frame
    Frame read_frame = dut_ifc->ReadFrame();
    if (CompareFrames(golden_frame, read_frame) == false)
    {
        test_result = (int) TestResult::Failed;
        TestControllerAgentEndTest(false);
    }
}


void test::TestBase::CheckNoRxFrame()
{
    if (dut_ifc->HasRxFrame())
    {
        TestMessage("DUT has received frame but it shouldnt!");
        test_result = false;
    }
}


void test::TestBase::CheckRecChange(int reference_rec, int delta)
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


void test::TestBase::CheckTecChange(int reference_tec, int delta)
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


void test::TestBase::WaitDutErrorActive()
{
    TestMessage("Waiting till DUT is error active...");
    while (dut_ifc->GetErrorState() != FaultConfState::ErrAct)
        usleep(100000);
    TestMessage("DUT is error active!");
}


void test::TestBase::ReconfigureDutBitTiming()
{
    dut_ifc->Disable();
    dut_ifc->ConfigureBitTiming(nominal_bit_timing, data_bit_timing);
    dut_ifc->Enable();
}


void test::TestBase::PushFramesToLowerTester(can::BitFrame &driver_bit_frame,
                                                 can::BitFrame &monitor_bit_frame)
{
    TestSequence *test_sequence;

    test_sequence = new TestSequence(this->dut_clock_period, driver_bit_frame, monitor_bit_frame);

#ifndef NDEBUG
    TestMessage(std::string(80, '*').c_str());
    TestMessage("Pushing sequences to lower tester...");
    TestMessage(std::string(80, '*').c_str());

    TestMessage("Driven sequence:");
    test_sequence->Print(true);

    TestMessage("Monitored sequence:");
    test_sequence->Print(false);

    TestMessage(std::string(80, '*').c_str());
#endif

    test_sequence->PushDriverValuesToSimulator();
    test_sequence->PushMonitorValuesToSimulator();

    delete test_sequence;
}


int test::TestBase::RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                                    [[maybe_unused]] const TestVariant &test_variant)
{
    return 0;
}


void test::TestBase::RunLowerTester(bool start_driver, bool start_monitor)
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


void test::TestBase::StartDriverAndMonitor()
{
    CanAgentMonitorStart();
    CanAgentDriverStart();
}


void test::TestBase::WaitForDriverAndMonitor()
{
    CanAgentMonitorWaitFinish();
    CanAgentDriverWaitFinish();
}


void test::TestBase::CheckLowerTesterResult()
{
    CanAgentCheckResult();
    CanAgentMonitorStop();
    CanAgentDriverStop();
    CanAgentMonitorFlush();
    CanAgentDriverFlush();
}

void test::TestBase::PrintTestInfo()
{
    TestMessage(std::string(80, '*').c_str());
    TestMessage("Test Name: %s", test_name.c_str());
    TestMessage("Number of variants: %d", test_variants.size());
    int num_elem_tests = 0;
    for (const auto &variant_tests : elem_tests)
        num_elem_tests += variant_tests.size();
    TestMessage("Total number of elementary tests: %d", num_elem_tests);
}

void test::TestBase::PrintElemTestInfo(ElementaryTest elem_test)
{
    TestMessage(std::string(80, '*').c_str());
    TestMessage("Elementary Test index: %d", elem_test.index_);
    //TestMessage("Elementary Test message: %s", elem_test.msg.c_str());
    TestMessage(std::string(80, '*').c_str());
}

void test::TestBase::PrintVariantInfo(TestVariant test_variant)
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

void test::TestBase::RandomizeAndPrint(Frame *frame)
{
    frame->Randomize();
    TestMessage("Test frame:");
    frame->Print();
}

void test::TestBase::FreeTestObjects()
{
    golden_frm.reset();
    golden_frm_2.reset();
    driver_bit_frm.reset();
    driver_bit_frm_2.reset();
    monitor_bit_frm.reset();
    monitor_bit_frm_2.reset();
}


size_t test::TestBase::CalcNumSamplePoints(bool nominal)
{
    int tmp;
    if (nominal)
        tmp = nominal_bit_timing.GetBitLenTQ();
    else
        tmp = data_bit_timing.GetBitLenTQ();

    // Minimal durations (in cycles):
    //  Nominal - TSEG1 = 5, TSEG2 = 3
    //  Data - TSEG1 = 3, TSEG2 = 2
    if (nominal)
    {
        if (nominal_bit_timing.brp_ == 1) {
            return tmp - 7;
        } else if (nominal_bit_timing.brp_ == 2) {
            return tmp - 4;
        } else if ((nominal_bit_timing.brp_ == 3) || (nominal_bit_timing.brp_ == 4)) {
            return tmp - 2;
        } else {
            return tmp - 1;
        }
    } else {
        if (data_bit_timing.brp_ == 1) {
            return tmp - 4;
        } else if (data_bit_timing.brp_ == 2) {
            return tmp - 2;
        } else {
            return tmp - 1;
        }
    }
}

BitTiming test::TestBase::GenerateBitTiming(const ElementaryTest &elem_test, bool nominal,
                                           size_t minimal_ph1)
{
    BitTiming new_bt;
    BitTiming *orig_bt;

    TestMessage("Generating new bit timing for elementary test index: %d", elem_test.index_);
    TestMessage("Bit timing type: %s", (nominal) ? "Nominal" : "Data");
    TestMessage("Target Minimal PH1 Length: %d", minimal_ph1);

    if (nominal)
        orig_bt = &backup_nominal_bit_timing;
    else
        orig_bt = &backup_data_bit_timing;

    size_t init_ph1 = GetDefaultMinPh1(orig_bt, nominal);

    if (init_ph1 < minimal_ph1)
        init_ph1 = minimal_ph1;

    TestMessage("Actual Minimal PH1 Length: %d", init_ph1);

    // If we have N Time Quanta bit time, then we can have at most N - 1 Sample point positions
    // regardless of bit time parameters / constraints. If we have more, this shows we have some
    // additional elementary tests, not just the ones for "each sample point". This situation
    // does not occur in standard, and if it happened, it was mostly an error in configuration
    // of number of elementary tests. Therefore we forbid this option.
    assert(((elem_test.index_ < orig_bt->GetBitLenTQ()) &&
             "Invalid test index, can't configure sample point!"));

    // Calculate new bit-rate from configured one. Have same bit-rate, but different sample point.
    // Shift sample point from "init_ph1" till the end.
    new_bt.brp_ = orig_bt->brp_;
    new_bt.prop_ = 0;
    new_bt.ph1_ = init_ph1 + elem_test.index_ - 1;
    new_bt.ph2_ = orig_bt->GetBitLenTQ() - new_bt.ph1_ - 1;

    // Handle cases where we add too many elementary tests and we would make PH2 equal to zero
    // or even less than zero causing underflow.
    if (new_bt.ph2_ == 0 || new_bt.ph2_ > 0xFFFF)
        new_bt.ph2_ = 1;

    // CTU CAN FD specific constraint for PH2 of nominal bit-rate
    // TODO: Handle this universally for all controllers!
    if (nominal && new_bt.ph2_ < 2)
        new_bt.ph2_ = 2;

    new_bt.sjw_ = std::min<size_t>(new_bt.ph2_, orig_bt->sjw_);

    TestMessage("Original bit timing without shifted sample point:");
    orig_bt->Print();

    TestMessage("New bit timing with shifted sample point:");
    new_bt.Print();

    return new_bt;
}