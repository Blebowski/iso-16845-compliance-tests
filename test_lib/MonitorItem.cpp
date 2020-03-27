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
                                   std::chrono::nanoseconds sampleRate)
{
    this->duration = duration;
    this->sampleRate = sampleRate;
    this->value = value;
    this->message = std::string();
    this->loggerSeverity = LoggerSeverity::WARNING;
}


test_lib::MonitorItem::MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                                   std::chrono::nanoseconds sampleRate, std::string message)
{
    this->duration = duration;
    this->sampleRate = sampleRate;
    this->value = value;
    this->message = message;
    this->loggerSeverity = LoggerSeverity::WARNING;
}


test_lib::MonitorItem::MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                                   std::chrono::nanoseconds sampleRate,
                                   LoggerSeverity loggerSeverity)
{
    this->duration = duration;
    this->sampleRate = sampleRate;
    this->value = value;
    this->message = std::string();
    this->loggerSeverity = loggerSeverity;
}


test_lib::MonitorItem::MonitorItem(std::chrono::nanoseconds duration, StdLogic value,
                                   std::chrono::nanoseconds sampleRate, std::string message,
                                   LoggerSeverity loggerSeverity)
{
    this->duration = duration;
    this->sampleRate = sampleRate;
    this->value = value;
    this->message = message;
    this->loggerSeverity = loggerSeverity;
}


bool test_lib::MonitorItem::hasMessage()
{
    if (message.size() > 0)
        return true;
    return false;
}


void test_lib::MonitorItem::print()
{
    if (hasMessage() == true)
        std::cout << message << std::endl;
    std::cout << "Value:    " << value << std::endl;
    std::cout << "Duration: " << duration.count() << " ns" << std::endl;
}