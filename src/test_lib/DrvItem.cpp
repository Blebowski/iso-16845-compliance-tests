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

#include <chrono>
#include <iostream>
#include <iomanip>

#include "DrvItem.h"


test::DrvItem::DrvItem(std::chrono::nanoseconds duration, StdLogic value):
    duration_(duration),
    value_(value)
{
    message_ = std::string();
}


test::DrvItem::DrvItem(std::chrono::nanoseconds duration, StdLogic value,
                       std::string message):
    duration_(duration),
    value_(value),
    message_(message)
{}


bool test::DrvItem::HasMessage()
{
    if (message_.size() > 0)
        return true;
    return false;
}


void test::DrvItem::Print()
{
    if (HasMessage() == true)
        std::cout << std::setw (20) << message_;

    if (value_ == StdLogic::LOGIC_0)
        std::cout << std::setw (20) << "0";
    else if (value_ == StdLogic::LOGIC_1)
        std::cout << std::setw (20) << "1";
    else
        std::cout << std::setw (20) << char(value_);

    std::cout << std::setw (20) << std::dec << duration_.count() << " ns\n";
}