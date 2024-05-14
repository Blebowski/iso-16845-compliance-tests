#ifndef ELEMENTARY_TEST_H
#define ELEMENTARY_TEST_H
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

#include <can_lib.h>

#include "test.h"

using namespace can;

/**
 * @namespace test
 * @class ElementaryTest
 * @brief Elementary test class
 *
 * Represents single Elementary test as described in ISO16845-1:2016.
 */
class test::ElemTest
{
    public:
        ElemTest(size_t index);
        ElemTest(size_t index, std::string msg);
        ElemTest(size_t index, std::string msg, FrameKind frame_kind);
        ElemTest(size_t index, FrameKind frame_kind);

        /* Index of the elementary test (starting from 1. This is the same
         * number as is after # in ISO116845!)
         */
        size_t index_;

        /* String to be printed when this elementary test starts */
        std::string msg_;

        /* Phase error (used during bit timing tests) */
        int e_;

        /* Frame type used by the test */
        FrameKind frame_kind_;
};

#endif