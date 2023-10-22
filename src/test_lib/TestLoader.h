#ifndef TEST_LOADER_H
#define TEST_LOADER_H
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

#include <memory>

#include "test.h"
#include "TestBase.h"


/**
 * @brief C++ test execution entry
 *
 * Main C++ test execution function. Forks of test thread. Called by PLI
 * callback when TB in digital simulator requests passing control to SW
 * test. Called in simulator context.
 *
 * @param test_name Name of SW testcase to run. Used to construct corresponding
 *                 test object.
 */
extern "C" void RunCppTest(char *test_name);


/**
 * @brief Construct test object
 *
 * Creates test object for given test name. Test object must be supported in
 * implementation of this function.
 *
 * @param name Name of SW test object to create.
 */
test::TestBase* ConstructTestObject(std::string name);


/**
 * Prints message to standard output. Message with "SW test" prefix is printed.
 *
 * @param message String to be printed.
 * @todo Implement support of varargs!
 */
void TestMessage(const char *fmt, ...);


/**
 * Prints message enclosed with line of "*".
 * @param message String to be printed.
 */
void TestBigMessage(std::string message, ...);

#endif