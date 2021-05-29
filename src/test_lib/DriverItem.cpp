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

#include <chrono>
#include <iostream>

#include "test_lib.h"
#include "DriverItem.h"


test_lib::DriverItem::DriverItem(std::chrono::nanoseconds duration, StdLogic value):
    duration_(duration),
    value_(value)
{
    message_ = std::string();
}


test_lib::DriverItem::DriverItem(std::chrono::nanoseconds duration, StdLogic value,
                                  std::string message):
    duration_(duration),
    value_(value),
    message_(message)
{}


bool test_lib::DriverItem::HasMessage()
{
    if (message_.size() > 0)
        return true;
    return false;
}


void test_lib::DriverItem::Print()
{
    if (HasMessage() == true)
        std::cout << message_ << std::endl;
    if (value_ == StdLogic::LOGIC_0)
        std::cout << "_";
    else if (value_ == StdLogic::LOGIC_1)
        std::cout << "¯";
    else
        std::cout << char(value_);

    std::cout << "Value:    " << (char)value_ << '\n';
    std::cout << "Duration: " << duration_.count() << " ns" << std::endl;
}