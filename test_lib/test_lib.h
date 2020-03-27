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
enum StdLogic : char
{
    LOGIC_0 = '0',  // Logic 0
    LOGIC_1 = '1',  // Logic 1
    LOGIC_H = 'H', // Pull up
    LOGIC_L = 'L',        // Pull down
    LOGIC_Z = 'Z',        // High impedance
    LOGIC_X = 'X',        // Logic X
    LOGIC_W = 'W',        // Weak signal
    LOGIC_U = 'U',        // Unknown
    LOGIC_DC = '-',       // Don't care
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