/**
 * TODO: License
 */

#include <iostream>
#include <thread>
#include <atomic>

#include "test_lib.h"
#include "TestLoader.h"

#include "TestBase.h"

/******************************************************************************
 * Implementations of compliance tests
 *****************************************************************************/
#include "../compliance_tests/TestDemo.cpp"


/******************************************************************************
 *****************************************************************************/

test_lib::TestBase *cppTest;
std::thread *testThread;


/******************************************************************************
 * Mapping of test name to Class representing the test.
 * This is simple workaround so that we don't have to implement some form
 * of reflection/factory solution which would convert us string to constructor
 *****************************************************************************/
test_lib::TestBase* constructTestObject(std::string name)
{
    test_lib::TestBase *testPtr = NULL;

    if (name == "base") {
        testPtr = new test_lib::TestBase();
    } else if (name == "demo") {
        testPtr = new test_lib::TestDemo();
    } else {
        std::cerr << "Unknown test name: " << name << std::endl;
    }

    return testPtr;
}


void testMessage(std::string message, ...)
{
    std::cout << "\033[1;92mSW test: \033[0m" << message << std::endl;
}


int cppTestThread(char *testName)
{
    int testRetVal;
    cppTest = constructTestObject(testName);
    testRetVal = cppTest->run();
    delete cppTest;

    return testRetVal;
}


void runCppTest(char* testName)
{
    testMessage(std::string(80, '*'));
    testMessage("Running C++ test: %s", testName);
    testMessage(std::string(80, '*'));

    testThread = new std::thread(cppTestThread, testName);

    // TODO: Destroy thread object when it ends!
}