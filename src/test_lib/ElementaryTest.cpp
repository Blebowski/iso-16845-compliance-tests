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
 * @date 25.8.2020
 *
 *****************************************************************************/

#include <chrono>
#include <string>
#include <list>
#include <memory>

#include "../can_lib/can.h"
#include "../can_lib/BitTiming.h"
#include "../can_lib/DutInterface.h"
#include "../can_lib/BitFrame.h"
#include "ElementaryTest.h"

#include "test_lib.h"

using namespace can;

test_lib::ElementaryTest::ElementaryTest(int index) :
    index_(index)
{
    msg_ = "Elementary test: ";
    msg_ += std::to_string(index);
}

test_lib::ElementaryTest::ElementaryTest(int index, std::string msg):
    index_(index),
    msg_(msg)
{}

test_lib::ElementaryTest::ElementaryTest(int index, std::string msg, FrameType frame_type):
    index_(index),
    msg_(msg),
    frame_type_(frame_type)
{}

test_lib::ElementaryTest::ElementaryTest(int index, FrameType frame_type):
    index_(index),
    frame_type_(frame_type)
{
    msg_ = "Elementary test: ";
    msg_ += std::to_string(index);
}