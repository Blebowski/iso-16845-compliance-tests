#ifndef DRIVER_ITEM_H
#define DRIVER_ITEM_H
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

#include <string>
#include <chrono>

#include "test_lib.h"

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
        DriverItem(std::chrono::nanoseconds duration, StdLogic value, std::string message);

        /**
         * @brief Checks if items has message printed by digital simulator when CAN agent starts
         *        driving this item.
         *
         * @return true if item has message, false otherwise
         */
        bool HasMessage();

        /**
         * @brief Print item
         */
        void Print();

        /**
         * Time for which the item is driven. When this is CAN bit, then this
         * represents length of the bit on CAN bus.
         */
        std::chrono::nanoseconds duration_;

        /**
         * Value which is driven by CAN agent driver.
         */
        StdLogic value_;

        /**
         * Message to be displayed by digital simulator when driving of item starts.
         */
        std::string message_;
};

#endif