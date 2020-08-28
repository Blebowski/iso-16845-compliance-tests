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

#include "test_lib.h"

#ifndef ELEMENTARY_TEST
#define ELEMENTARY_TEST

using namespace can;

/**
 * @namespace test_lib
 * @class ElementaryTest
 * @brief Elementary test class
 * 
 * Represents single Elementary test as described in ISO16845-1:2016.
 */
class test_lib::ElementaryTest
{
    public:
        ElementaryTest(int index);
        ElementaryTest(int index, std::string msg);
        ElementaryTest(int index, std::string msg, FrameType frame_type);
        ElementaryTest(int index, FrameType frame_type);

        /* Index of the elementary test (starting from 1. This is the same
         * number as is after # in ISO116845!)
         */
        int index;

        /* String to be printed when this elementary test starts */
        std::string msg;

        /* Phase error (used during bit timing tests) */
        int e;

        /* Frame type used by the test */
        FrameType frame_type;
};

#endif