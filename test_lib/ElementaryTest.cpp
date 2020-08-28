/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
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

test_lib::ElementaryTest::ElementaryTest(int index)
{
    this->index = index;
    msg = "Elementary test: ";
    msg += std::to_string(index);
}

test_lib::ElementaryTest::ElementaryTest(int index, std::string msg)
{
    this->index = index;
    this->msg = msg;
}

test_lib::ElementaryTest::ElementaryTest(int index, std::string msg, FrameType frame_type)
{
    this->index = index;
    this->msg = msg;
    this->frame_type = frame_type;
}

test_lib::ElementaryTest::ElementaryTest(int index, FrameType frame_type)
{
    this->index = index;
    msg = "Elementary test: ";
    msg += std::to_string(index);
    this->frame_type = frame_type;
}