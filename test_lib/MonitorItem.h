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
                    std::chrono::nanoseconds sample_rate);
        MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                    std::chrono::nanoseconds sample_rate, std::string message);

        /**
         * Checks if item has message which will be printed by digital simulator
         * when CAN agent starts monitoring this item.
         * 
         * @returns true if item has message, false otherwise
         */
        bool HasMessage();

        /**
         * Prints item
         */
        void Print();

        /**
         * Time for which the item is monitored. When this is CAN bit, then this
         * represents length of the bit on CAN bus.
         */
        std::chrono::nanoseconds duration;

        /**
         * Sample rate of this item. Indicates how often during monitoring of
         * item CAN agent monitor checks value of can_tx.
         */
        std::chrono::nanoseconds sample_rate;

        /**
         * Value towards which can_tc shall be checked by CAN agent monitor during monitoring.
         */
        StdLogic value;

        /**
         * Message to be displayed by digital simulator when monitoring of item
         * starts.
         */
        std::string message;
};

#endif