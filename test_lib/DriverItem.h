/**
 * TODO: License
 */

#include <string>
#include <chrono>

#include "test_lib.h"

#ifndef DRIVER_ITEM
#define DRIVER_ITEM

class test_lib::DriverItem
{
    public:
        DriverItem(std::chrono::nanoseconds duration, StdLogic value);
        DriverItem(std::chrono::nanoseconds duration, StdLogic value,
                   std::string message);

        bool hasMessage();

        void print();

        // Time for which the item is driven
        std::chrono::nanoseconds duration;

        // Value that is driven
        StdLogic value;

        // Message to be displayed when driving starts
        std::string message;
};

#endif