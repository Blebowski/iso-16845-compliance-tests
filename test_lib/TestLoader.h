/**
 * TODO: License
 */

#include "test_lib.h"

#include "test_lib.h"
#include "TestBase.h"

#ifndef TEST_LOADER
#define TEST_LOADER

/**
 * 
 */
extern "C" void *runCppTest(char *testName);

/**
 * 
 */
test_lib::TestBase *constructTestObject(std::string name);


/**
 * TODO: Do test message via "va_arg" so that strings can be passed with formatting!!!
 */
void test_message(std::string message, ...);

#endif