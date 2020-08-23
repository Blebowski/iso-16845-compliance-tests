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


bool test_lib::DriverItem::HasMessage()
{
    if (message.size() > 0)
        return true;
    return false;
}


void test_lib::DriverItem::Print()
{
    if (HasMessage() == true)
        std::cout << message << std::endl;
    if (value == StdLogic::LOGIC_0)
        std::cout << "_";
    else if (value == StdLogic::LOGIC_1)
        std::cout << "¯";
    else
        std::cout << char(value);

    std::cout << "Value:    " << (char)value << std::endl;
    std::cout << "Duration: " << duration.count() << " ns" << std::endl;
}