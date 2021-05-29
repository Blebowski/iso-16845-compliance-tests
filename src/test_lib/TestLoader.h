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

#include <memory>

#include "test_lib.h"

#include "test_lib.h"
#include "TestBase.h"

#ifndef TEST_LOADER
#define TEST_LOADER


/**
 * @brief C++ test execution entry
 * 
 * Main C++ test execution function. Forks of test thread. Called by VPI
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
test_lib::TestBase* ConstructTestObject(std::string name);


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