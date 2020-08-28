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

#include <chrono>
#include <list>
#include <vector>

#include "test_lib.h"
#include "../can_lib/can.h"
#include "../can_lib/BitTiming.h"
#include "../can_lib/DutInterface.h"
#include "ElementaryTest.h"

#ifndef TEST_BASE
#define TEST_BASE

using namespace can;

/**
 * @namespace test_lib
 * @class TestBase
 * @brief Test Base class
 * 
 * Contains common attributes used by all tests.
 * 
 * @note Each test should inherit from this class.
 * @note Each test should call TestBase::run() in the beginning of its run
 *       function.
 */
class test_lib::TestBase
{
    public:

        /**
         * Each class inheriting from TestBase should call this constructor
         * before any other actions.
         */
        TestBase();

        /**********************************************************************
         * Test configuration.
         * 
         * Path of test configuration is like so:
         *  1. YAML config file.
         *  2. VUnit applies it to TB top generics. Generics are propagated to
         *     Test controller agent.
         *  3. Test reads this configuration from test controller agent via VPI! 
         **********************************************************************/

        /**
         * Clock period to be set in TB.
         */
        std::chrono::nanoseconds dut_clock_period;

        /**
         * CAN bus Bit timing setting for nominal/data bit rate.
         */
        BitTiming nominal_bit_timing;
        can::BitTiming data_bit_timing;

        /**
         * Test name
         */
        std::string test_name;

        /**
         * Pointer to DUT Interface object. Object created when TestBase object
         * is created. Used to access DUT by tests.
         * TODO: Replace with unique pointer!
         */
        can::DutInterface* dut_ifc;

        /**
         * Version of CAN FD protocol that should be used for the test.
         */
        can::CanVersion dut_can_version;

        /**
         * Number of elementary tests (usually within single test variant)!
         */
        int num_elem_tests;

        /**
         * Test variants to be run. E.g. if DUT is CAN FD Enabled, CAN 2.0 and CAN FD
         * variants needs to be run in most cases.
         */
        std::vector<TestVariant> test_variants;

        /**
         * 
         */
        std::vector<std::vector<ElementaryTest>> elem_tests;

        /**
         * Test result
         */
        int test_result;

        /**
         * Seed from TB
         */
        int seed;

        /********************************************************************************
         * Data used during tests
         ********************************************************************************/

        /* Metadata and flags */
        FrameType frame_type;
        std::unique_ptr<FrameFlags> frame_flags;
        std::unique_ptr<FrameFlags> frame_flags_2;

        /* Frames used during test case. */
        std::unique_ptr<Frame> golden_frm;
        std::unique_ptr<Frame> golden_frm_2;

        /* Bit Frames used by driver (in most of the tests on is used) */
        std::unique_ptr<BitFrame> driver_bit_frm;
        std::unique_ptr<BitFrame> driver_bit_frm_2;

        /* Bit Frame used by monitor (in most of the tests on is used) */
        std::unique_ptr<BitFrame> monitor_bit_frm;
        std::unique_ptr<BitFrame> monitor_bit_frm_2;

        /*********************************************************************************
         * THESE ARE LEGACY AND WILL BE DELETED when all tests are cleaned to use uniques!
         ********************************************************************************/
        can::Frame *golden_frame;
        can::BitFrame *driver_bit_frame;
        can::BitFrame *monitor_bit_frame;
        can::BitFrame *driver_bit_frame_2;
        can::BitFrame *monitor_bit_frame_2;


        /** 
         * Obtains frame type based on test variant.
         */
        can::FrameType GetDefaultFrameType(TestVariant &variant);

        /********************************************************************************
         * Test execution functions
         ********************************************************************************/

        /**
         * Configuration function. Shall contain TB setup which is test specific.
         */
        virtual void ConfigureTest();

        /**
         * Runs base test (defualt config) + 'ConfigureTest' to execute test specific
         * config.
         */
        void SetupTestEnvironment();

        /**
         * Runs test.
         */
        virtual int Run();

        /**
         * Cleans test environment. Notifies TestControllent (in simulation) with
         * 'test_result'. Returns value based on test result.
         * Should be called at the end of tests 'Run' method.
         */
        TestResult FinishTest();
        TestResult FinishTest(TestResult test_result);

        /********************************************************************************
         * Auxiliarly functions used during Run of the test.
         ********************************************************************************/

        /**
         * Fills 'test_variants' and creates vectors for each variant in 'elem_tests'.
         */
        void FillTestVariants(VariantMatchingType match_type);

        /**
         * 
         */
        std::unique_ptr<BitFrame> ConvertBitFrame(Frame &golden_frame);

        /**
         * Compares two frames.
         * @returns true if frames are equal, false otherwise
         */
        bool CompareFrames(can::Frame &expected_frame, can::Frame &real_frame);

        /**
         * Check
         */
        void CheckRxFrame(Frame &golden);

        /**
         * Loads Bit frames to driver and monitor. Pushes it as driver/monitor FIFO items.
         * @param driver_frame bit frame to be loaded into driver
         * @param monitor_frame bit frame to be loaded into monitor
         */
        void PushFramesToLowerTester(can::BitFrame &driver_frame, can::BitFrame &monitor_frame);

        /**
         * Starts driver and/or monitor and waits till they are finished.
         * @param start_driver driver shall be started and waited on
         * @param start_monitor monitor shall be started and waited on
         */
        void RunLowerTester(bool start_driver, bool start_monitor);

        /**
         * Starts driver and monitor. 
         */
        void StartDriverAndMonitor(void);

        /*
         * Waits till driver and monitor are finished with driving.
         */
        void WaitForDriverAndMonitor(void);

        /**
         * Checks lower tester result. If monitor in Lower tester contains mismatches during last
         * monitoring, it prints error report to simulation log!
         */
        void CheckLowerTesterResult();

        /**********************************************************************
         * Print functions. To be used during test
         *********************************************************************/

        /**
         * TODO
         */
        void PrintTestInfo();

        /**
         * 
         */
        void PrintElemTestInfo(ElementaryTest elem_test);

        /**
         * 
         */
        void PrintVariantInfo(TestVariant test_variant);

        /**
         * Randomizes and prints 
         */
        void RandomizeAndPrint(Frame *frame);

        /**
         * Deletes: golden frame, driver bit frame and monitor bit frame
         * TODO: Replace with unique pointers!!
         */
        void DeleteCommonObjects();
};

#endif