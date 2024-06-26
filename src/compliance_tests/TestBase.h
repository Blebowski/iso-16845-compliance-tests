#ifndef TEST_BASE_H
#define TEST_BASE_H
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

#include <chrono>
#include <list>
#include <vector>

#include <can_lib.h>
#include <test_lib.h>

#define TEST_ASSERT(cond, msg) TestAssertFnc(cond, msg, __FILE__, __LINE__);

/**
 * @namespace test
 * @class TestBase
 * @brief Test Base class
 *
 * Contains common attributes used by all tests.
 *
 * @note Each test should inherit from this class.
 * @note Each test should call TestBase::run() in the beginning of its run
 *       function.
 */
class test::TestBase
{
    public:

        /**
         * Each class inheriting from TestBase should call this constructor
         * before any other actions.
         */
        TestBase();

        /**
         * Destructor must be virtual
         */
        virtual ~TestBase();

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
        std::chrono::nanoseconds dut_clk_period;

        /**
         * Input delay of DUT. Corresponds to time it takes to signal from can_rx
         * to be processed by CAN protocol controller. This delay typically includes
         * delay to resynchronize digital signal (two flops = two cycles). Also,
         * if long wiring leads to DUT from IUT, signal propagation through CAN RX
         * from point where IUT, to input of DUT shall be included.
         *
         * Should be set in unit of IUTs clock cycle. E.g. if clock cycle is 5 ns,
         * and IUTs input delay is 15 ns, put 3 here. Value rounds down.
         * TODO: Check rounding down is OK!
         */
        size_t dut_input_delay;

        /**
         * Information processing time of DUT (in minimal time quanta = clock cycles)
         */
        size_t dut_ipt;

        /**
         * Maximal size of Secondary sample point delay (in clock cycles)
         */
        size_t dut_max_secondary_sample;

        /**
         * CAN Bus bit timing. By default contains bit timing queryied from TB.
         * If test requires other bit timing, it will modify it!
         */
        can::BitTiming nbt;
        can::BitTiming dbt;

        /**
         * Backup bit timing. Always contains bit timing queryied from TB.
         */
        can::BitTiming bckp_nbt;
        can::BitTiming bckp_dbt;

        /**
         * Test name
         */
        std::string test_name;

        /**
         * Pointer to DUT Interface object. Object created when TestBase object
         * is created. Used to access DUT by tests.
         */
        can::DutInterface* dut_ifc;

        /**
         * Version of CAN FD protocol that should be used for the test.
         */
        can::CanVersion dut_can_version;

        /**
         * Test variants to be run. E.g. if DUT is CAN FD Enabled, CAN 2.0 and CAN FD
         * variants needs to be run in most cases.
         */
        std::vector<TestVariant> test_variants;

        /**
         * Elementary test cases to be ran during the test.
         */
        std::vector<std::vector<ElemTest>> elem_tests;

        /**
         * Test result
         */
        int test_result;

        /**
         * Seed from TB
         */
        int seed;

        /**
         * Number of Stuff bits within one Test variant. Used during tests which
         * contain single elementary test, with clause like: "each stuff bit will be
         * tested"
         */
        int stuff_bits_in_variant = 0;

        /**
         * Error data byte. Used in tests where error frame shall be invoked. Contains
         * 0x80 and test shall corrupt its 7 data bit (should be recessive stuff bit).
         */
        uint8_t error_data = 0x80;

        /********************************************************************************
         * Data used during tests
         ********************************************************************************/

        /* Metadata and flags */
        FrameKind frm_kind;
        std::unique_ptr<can::FrameFlags> frm_flags;
        std::unique_ptr<can::FrameFlags> frm_flags_2;

        /* Frames used during test case. */
        std::unique_ptr<can::Frame> gold_frm;
        std::unique_ptr<can::Frame> gold_frm_2;

        /* Bit Frames used by driver (in most of the tests on is used) */
        std::unique_ptr<can::BitFrame> drv_bit_frm;
        std::unique_ptr<can::BitFrame> drv_bit_frm_2;
        std::unique_ptr<can::BitFrame> drv_bit_frm_3;
        std::unique_ptr<can::BitFrame> drv_bit_frm_4;

        /* Bit Frame used by monitor (in most of the tests on is used) */
        std::unique_ptr<can::BitFrame> mon_bit_frm;
        std::unique_ptr<can::BitFrame> mon_bit_frm_2;
        std::unique_ptr<can::BitFrame> mon_bit_frm_3;
        std::unique_ptr<can::BitFrame> mon_bit_frm_4;

        /* REC / TEC counters */
        int rec_old = 0;
        int rec_new = 0;
        int tec_old = 0;
        int tec_new = 0;

        // Assertion counters
        int failed_assertions = 0;

        /**
         * Obtains frame type based on test variant.
         */
        can::FrameKind GetDefFrameKind(TestVariant &variant);

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
        void SetupTestEnv();

        /**
         * Setup VIP monitor (in HDL simulation) for simulations where IUT starts as
         * transmitter (8.x tests). This includes following:
         *   1. Trigger on CAN_TX falling edge.
         *   2. 0 ns input delay of monitor after triggering.
         *   3. Wait for monitor item.
         */
        void SetupMonitorTxTests();

        /**
         * Runs test.
         */
        virtual int Run();

        /**
         * Runs single elementary test.
         */
        virtual int RunElemTest(const ElemTest &elem_test, const TestVariant &test_variant);

        /**
         *
         */
        virtual int FinishElemTest();

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
        void FillTestVariants(VariantMatchType match_type);

        /**
         * Adds elementary test to a test variant.
         */
        void AddElemTest(TestVariant test_variant, ElemTest &&elem_test);

        /**
         * Adds elementary test for each sample point within a given bit-rate.
         * Elemetary test object is created by this function.
         * @param test_variant Test variant in which the test will be added
         * @param nominal True - One test per nominal bit rate sample point
         *                False - One test per data bit rate sample point
         * @param frame_kind Type of frame assigned to each added test.
         */
        void AddElemTestForEachSP(TestVariant test_variant, bool nominal, FrameKind frame_kind);

        /**
         *
         */
        BitTiming GenerateSPForTest(const ElemTest &elem_test, bool nominal);

        /**
         * TODO
         */
        BitTiming GenerateSPForTest(const ElemTest &elem_test, bool nominal,
                                    size_t minimal_ph1);

        /**
         * Generates bit sequence (bit representation) of CAN frame from frame.
         * Standard bit sequence contains ACK recessive (as if frame was transmitted).
         */
        std::unique_ptr<BitFrame> ConvBitFrame(Frame &golden_frame);

        /**
         * Compares two frames.
         * @returns true if frames are equal, false otherwise
         */
        bool CompareFrames(can::Frame &expected_frame, can::Frame &real_frame);

        /**
         * @returns random bit type within a bit field.
         */
        BitKind GetRandomBitType(FrameKind frame_kind, IdentKind ident_kind,
                                 BitField bit_field);

        /**
         * Reads frame from IUT and checks that it is equal to given frame. Sets
         * 'test_result' to false if not.
         * @param gold Frame to compare with the one which is read from DUT.
         */
        void CheckRxFrame(Frame &gold);

        /**
         * Checks that IUT has no frame received. Sets 'test_result' to false if
         * IUT has a frame in its RX Buffer.
         */
        void CheckNoRxFrame();

        /**
         * Reads REC counter from IUT and checks it was changed in comparison to
         * reference value. Sets 'test_result' to false if not.
         * @param ref_rec Reference value (old value)
         * @param delta Change of REC expected. Positive values check that REC
         *              was incremented, negative that it was decremented.
         */
        void CheckRecChange(int ref_rec, int delta);

        /**
         * Reads TEC counter from IUT and checks it was changed in comparison to
         * reference value. Sets 'test_result' to false if not.
         * @param ref_tec Reference value (old value)
         * @param delta Change of REC expected. Positive values check that TEC
         *              was incremented, negative that it was decremented.
         */
        void CheckTecChange(int ref_tec, int delta);

        /**
         * Poll DUTs Fault confinement state until it becomes error active.
         */
        void WaitDutErrAct();

        /**
         * Disables DUT, configures its bit timing and, re-enables it.
         */
        void ReconfDutBitTiming();

        /**
         * Loads Bit frames to driver and monitor. Pushes it as driver/monitor FIFO items.
         * @param drv_frame bit frame to be loaded into driver
         * @param mon_frame bit frame to be loaded into monitor
         */
        void PushFramesToLT(can::BitFrame &drv_frame, can::BitFrame &mon_frame);

        /**
         * Starts driver and/or monitor and waits till they are finished.
         * @param start_driver driver shall be started and waited on
         * @param start_monitor monitor shall be started and waited on
         */
        void RunLT(bool start_driver, bool start_monitor);

        /**
         * Starts driver and monitor.
         */
        void StartDrvAndMon(void);

        /*
         * Waits till driver and monitor are finished with driving.
         */
        void WaitForDrvAndMon(void);

        /**
         * Checks lower tester result. If monitor in Lower tester contains mismatches during last
         * monitoring, it prints error report to simulation log!
         */
        void CheckLTResult();

        /*
         * Assertion check that will not actually cause simulation crash.
         */
        void TestAssertFnc(bool condition, const char *msg, const char *file, const int line);

        /**********************************************************************
         * Print functions.
         *********************************************************************/

        /**
         * TODO
         */
        void PrintTestInfo();

        /**
         *
         */
        void PrintElemTestInfo(ElemTest elem_test);

        /**
         *
         */
        void PrintVariantInfo(TestVariant test_variant);

        /**
         * Randomizes and prints
         */
        void RandomizeAndPrint(Frame *frame);

        /**
         * Forces erase of all test specific pointers. This is desirable to do
         * each iteration.
         */
        void FreeTestObjects();

    private:
        /**
         * Calculates number of possible sample points per bit-rate.
         * @note CTU CAN FDs limit of min(TSEG1) = 3 clock cycles is taken into account.
         */
        size_t CalcNumSPs(bool nominal);

        /**
         * Returns minimal Ph1 duration based on current bit-rate configuration. Minimal
         * Ph1 is chosen such that minimal bit-rate of IUT is respected!
         */
        size_t GetDefaultMinPh1(BitTiming *orig_bt, bool nominal);

        /**
         * Generates bit-rate with sample point specific for elementary test.
         * @param elem_test Elementary test which is being run
         * @param bit_timing Original bit timing configuration
         *
         * Sample point is configured like so:
         *  1. Default bit-rate is taken
         *  2. Sample point is shifted by index of elementary test:
         *      test 1 -> PH1 = minimal lenght as given by IUT constraints!
         *      test 2 -> PH1 = 2
         *      ...
         *     PROP = 0, PH2 is set to achieve the same length of bit as in
         *     default bit timing config.
         *
         * DUT considerations are taken into account!
         */
        BitTiming GenerateBitTiming(const ElemTest &elem_test, bool is_nominal, size_t minimal_ph1);
};

#endif