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

#include <string>
#include <chrono>

#include "test_lib.h"

#ifndef MONITOR_ITEM
#define MONITOR_ITEM

/**
 * @namespace test_lib
 * @class MonitorItem
 * @brief CAN Agent monitor item
 * 
 * Represents single item to be monitored by CAN Agent monitor.
 */
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

        /**
         * @brief Checks if items has message
         * 
         * Checks if item has message which will be printed by digital simulator
         * when CAN agent starts monitoring this item.
         * 
         * @return true if item has message, false otherwise
         */
        bool hasMessage();

        /**
         * @brief Print item
         */
        void print();

        /**
         * Time for which the item is monitored. When this is CAN bit, then this
         * represents length of the bit on CAN bus.
         */
        std::chrono::nanoseconds duration;

        /**
         * Sample rate for this item. Indicates how often during monitoring of
         * item CAN agent monitor checks value of can_tx.
         */
        std::chrono::nanoseconds sampleRate;

        /**
         * Value which is monitored by CAN agent monitor.
         */
        StdLogic value;

        /**
         * Message to be displayed by digital simulator when monitoring of item
         * starts.
         */
        std::string message;

        /**
         * Severity of message to be printed when mismatch is monitored.
         */
        LoggerSeverity loggerSeverity;
};

#endif