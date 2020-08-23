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

#ifndef TEST_LIB
#define TEST_LIB

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
         * CAN 2.0 -> CAN 2.0
         * CAN FD Tolerant -> CAN FD Tolerant
         * CAN FD Enabled -> CAN FD Enabled
         */
        OneToOne
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