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
#include "MonitorItem.h"


test_lib::MonitorItem::MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                                   std::chrono::nanoseconds sample_rate)
{
    this->duration = duration;
    this->sample_rate = sample_rate;
    this->value = value;
    this->message = std::string();
}


test_lib::MonitorItem::MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                                   std::chrono::nanoseconds sample_rate, std::string message)
{
    this->duration = duration;
    this->sample_rate = sample_rate;
    this->value = value;
    this->message = message;
}


bool test_lib::MonitorItem::HasMessage()
{
    if (message.size() > 0)
        return true;
    return false;
}


void test_lib::MonitorItem::Print()
{
    if (HasMessage())
        std::cout << message << std::endl;
    std::cout << "Value:    " << (char)value << std::endl;
    std::cout << "Duration: " << duration.count() << " ns" << std::endl;
}