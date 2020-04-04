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
 * test.
 * 
 * @param testName Name of SW testcase to run. Used to construct corresponding
 *                 test object.
 */
extern "C" void runCppTest(char *testName);


/**
 * @brief Construct test object
 * 
 * Creates test object for given test name. Test object must be supported in
 * implementation of this function.
 * 
 * @param testName Name of SW test object to create.
 */
test_lib::TestBase *constructTestObject(std::string name);


/**
 * @brief Print message to standard output.
 * 
 * Message with "SW test" prefix is printed.
 * 
 * @todo Implement support of varargs!
 * 
 * @param message String to be printed
 */
void testMessage(std::string message, ...);


/**
 * @brief TODO
 */
void testBigMessage(std::string message, ...);

#endif