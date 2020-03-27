/**
 * TODO: License
 */

#include <string>
#include <chrono>

#include "test_lib.h"

#ifndef MONITOR_ITEM
#define MONITOR_ITEM

class test_lib::MonitorItem
{
    public:
        MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                    std::chrono::nanoseconds sampleRate);
        MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                    std::chrono::nanoseconds sampleRate, std::string message);
        MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                    std::chrono::nanoseconds sampleRate, LoggerSeverity loggerSeverity);
        MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                    std::chrono::nanoseconds sampleRate, std::string message,
                    LoggerSeverity loggerSeverity);

        bool hasMessage();

        void print();

        // Time for which the item is monitored
        std::chrono::nanoseconds duration;

        // Sample rate used to sample and compare this item
        std::chrono::nanoseconds sampleRate;

        // Value that is monitored
        StdLogic value;

        // Message to be displayed when monitoring starts
        std::string message;

        // Severity of message to be printed when mismatch is monitored
        LoggerSeverity loggerSeverity;
};

#endif