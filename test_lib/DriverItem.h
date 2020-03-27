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

#ifndef DRIVER_ITEM
#define DRIVER_ITEM

/**
 * @namespace test_lib
 * @class DriverItem
 * @brief CAN Agent driver item
 * 
 * Represents single item to be driver by CAN Agent driver.
 */
class test_lib::DriverItem
{
    public:
        DriverItem(std::chrono::nanoseconds duration, StdLogic value);
        DriverItem(std::chrono::nanoseconds duration, StdLogic value,
                   std::string message);

        /**
         * @brief Checks if items has message
         * 
         * Checks if item has message which will be printed by digital simulator
         * when CAN agent starts driving this item.
         * 
         * @return true if item has message, false otherwise
         */
        bool hasMessage();

        /**
         * @brief Print item
         */
        void print();

        /**
         * Time for which the item is driven. When this is CAN bit, then this
         * represents length of the bit on CAN bus.
         */
        std::chrono::nanoseconds duration;

        /**
         * Value which is driven by CAN agent driver.
         */
        StdLogic value;

        /**
         * Message to be displayed by digital simulator when driving of item
         * starts.
         */
        std::string message;
};

#endif