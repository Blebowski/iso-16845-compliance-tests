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
    this->duration_ = duration;
    this->sample_rate_ = sample_rate;
    this->value_ = value;
    this->message_ = std::string();
}


test_lib::MonitorItem::MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                                   std::chrono::nanoseconds sample_rate, std::string message)
{
    this->duration_ = duration;
    this->sample_rate_ = sample_rate;
    this->value_ = value;
    this->message_ = message;
}


bool test_lib::MonitorItem::HasMessage()
{
    if (message_.size() > 0)
        return true;
    return false;
}


void test_lib::MonitorItem::Print()
{
    if (HasMessage())
        std::cout << message_ << std::endl;
    std::cout << "Value:    " << (char)value_ << std::endl;
    std::cout << "Duration: " << duration_.count() << " ns" << std::endl;
}