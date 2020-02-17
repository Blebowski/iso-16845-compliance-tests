/**
 * TODO: License
 */

#include <chrono>
#include <iostream>

#include "test_lib.h"
#include "DriverItem.h"


test_lib::DriverItem::DriverItem(std::chrono::nanoseconds duration, StdLogic value)
{
    this->duration = duration;
    this->value = value;
    this->message = std::string();
}


test_lib::DriverItem::DriverItem(std::chrono::nanoseconds duration, StdLogic value,
                                  std::string message)
{
    this->duration = duration;
    this->value = value;
    this->message = message;
}


bool test_lib::DriverItem::hasMessage()
{
    if (message.size() > 0)
        return true;
    return false;
}


void test_lib::DriverItem::print()
{
    if (hasMessage() == true)
        std::cout << message << std::endl;
    if (value == 0)
        std::cout << "_";
    else
        std::cout << "¯";
    //std::cout << "Value:    " << value << std::endl;
    //std::cout << "Duration: " << duration.count() << " ns" << std::endl;
}