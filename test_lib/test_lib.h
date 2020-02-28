/*
 * TODO: License
 */

#ifndef TEST_LIB
#define TEST_LIB

namespace test_lib
{
    enum StdLogic
    {
        LOGIC_0,    // Logic 0
        LOGIC_1,    // Logic 1
        LOGIC_H,    // Pull up
        LOGIC_L,    // Pull down
        LOGIC_Z,    // High impedance
        LOGIC_X,    // Logic X
        LOGIC_W,    // Weak signal
        LOGIC_U,    // Unknown
        LOGIC_Y,    // Don't care
    };

    enum LoggerSeverity
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    enum MonitorTrigger
    {
        TODO
    };

    enum SequenceType
    {
        DRIVER_SEQUENCE,
        MONITOR_SEQUENCE
    };

    class DriverItem;
    class MonitorItem;
    class TestSequence;

    class TestBase;
    class TestDemo;

} // namespace test_lib

#endif