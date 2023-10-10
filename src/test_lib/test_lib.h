#ifndef TEST_LIB_H
#define TEST_LIB_H
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

namespace test_lib
{
    enum class StdLogic : char
    {
        LOGIC_0 = '0',      /* Logic 0 */
        LOGIC_1 = '1',      /* Logic 1 */
        LOGIC_H = 'H',      /* Pull up */
        LOGIC_L = 'L',      /* Pull down */
        LOGIC_Z = 'Z',      /* High impedance */
        LOGIC_X = 'X',      /* Logic X */
        LOGIC_W = 'W',      /* Weak signal */
        LOGIC_U = 'U',      /* Unknown */
        LOGIC_DC = '-',     /* Don't care */
    };

    enum class SequenceType
    {
        DRIVER_SEQUENCE,
        MONITOR_SEQUENCE
    };

    enum class TestVariant
    {
        Common,             /* Common for FD Enabled, Tolerant, 2.0 implementations */
        Can_2_0,            /* CAN 2.0 only */
        CanFdTolerant,      /* CAN FD Tolerant */
        CanFdEnabled        /* CAN FD Enabled */
    };

    /*
     * Mappings of DUT type to test variants. Some tests e.g. require run of CAN FD Enabled variant
     * for CAN FD Enabled node only. Other tests require run of CAN FD Enabled and CAN 2.0 variants
     * for CAN FD enabled node.
     */
    enum class VariantMatchingType
    {
        /*
         * CAN 2.0          -> CAN 2.0
         * CAN FD Tolerant  -> CAN FD Tolerant
         * CAN FD Enabled   -> CAN FD Enabled
         */
        OneToOne,

        /*
         * Any DUT version to single common variant.
         */
        Common,

        /*
         * CAN 2.0         -> Common variant only
         * CAN FD Tolerant -> Common variant only
         * CAN FD Enabled  -> Common variant (with FDF = 0) + FD variant (FDF = 1)
         *
         * This is the most frequently used mapping between DUT version and variants!
         */
        CommonAndFd,

        /**
         * Classical CAN   -> Classical CAN variant
         * CAN FD Enabled  -> CAN FD Enabled variant
         * CAN FD Tolerant -> No tests
         */
        ClassicalAndFdEnabled,

        /**
         * Classical CAN   -> No tests
         * CAN FD Tolerant -> FD Tolerant variant
         * CAN FD Enabled  -> FD Enabled variant
         *
         */
        FdTolerantFdEnabled,

        /*
         * Classical CAN    -> Classical variant
         * CAN FD tolerant  -> CAN FD Tolerant variant
         * CAN FD enabled   -> CAN FD Tolerant + CAN FD enabled variant
         */
        ClassicalFdCommon,

        /*
         * Classical CAN    -> No tests
         * CAN FD tolerant  -> No tests
         * CAN FD Enabled   -> CAN FD Tolerant variant
         */
        CanFdEnabledOnly
    };

    enum class TestResult : int {
        Passed  = 0,
        Failed  = 1,
        Skipped = 2
    };

    class DriverItem;
    class MonitorItem;
    class TestSequence;

    class TestBase;
    class ElementaryTest;
    class TestDemo;

} // namespace test_lib

#endif