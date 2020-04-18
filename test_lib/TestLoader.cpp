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

#include "../compliance_tests/TestIso_7_1_1.cpp"
#include "../compliance_tests/TestIso_7_1_4.cpp"
#include "../compliance_tests/TestIso_7_1_5.cpp"
#include "../compliance_tests/TestIso_7_1_8.cpp"
#include "../compliance_tests/TestIso_7_1_9.cpp"
#include "../compliance_tests/TestIso_7_1_12.cpp"

#include "../compliance_tests/TestIso_7_2_1.cpp"
#include "../compliance_tests/TestIso_7_2_6.cpp"
#include "../compliance_tests/TestIso_7_2_7.cpp"
#include "../compliance_tests/TestIso_7_2_9.cpp"
#include "../compliance_tests/TestIso_7_2_10.cpp"
#include "../compliance_tests/TestIso_7_2_11.cpp"

#include "../compliance_tests/TestIso_7_3_1.cpp"
#include "../compliance_tests/TestIso_7_3_2.cpp"
#include "../compliance_tests/TestIso_7_3_3.cpp"
#include "../compliance_tests/TestIso_7_3_4.cpp"

#include "../compliance_tests/TestIso_7_4_1.cpp"
#include "../compliance_tests/TestIso_7_4_2.cpp"
#include "../compliance_tests/TestIso_7_4_4.cpp"
#include "../compliance_tests/TestIso_7_4_5.cpp"

#include "../compliance_tests/TestIso_7_6_1.cpp"
#include "../compliance_tests/TestIso_7_6_2.cpp"
#include "../compliance_tests/TestIso_7_6_3.cpp"
#include "../compliance_tests/TestIso_7_6_4.cpp"
#include "../compliance_tests/TestIso_7_6_5.cpp"
#include "../compliance_tests/TestIso_7_6_6.cpp"
#include "../compliance_tests/TestIso_7_6_7.cpp"


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
    } else if (name == "iso_7_1_1") {
        testPtr = new TestIso_7_1_1();
    } else if (name == "iso_7_1_4") {
        testPtr = new TestIso_7_1_4();
    } else if (name == "iso_7_1_5") {
        testPtr = new TestIso_7_1_5();
    } else if (name == "iso_7_1_8") {
        testPtr = new TestIso_7_1_8();
    } else if (name == "iso_7_1_9") {
        testPtr = new TestIso_7_1_9();
    } else if (name == "iso_7_1_12") {
        testPtr = new TestIso_7_1_12();
    
    } else if (name == "iso_7_2_1") {
        testPtr = new TestIso_7_2_1();
    } else if (name == "iso_7_2_6") {
        testPtr = new TestIso_7_2_6();
    } else if (name == "iso_7_2_7") {
        testPtr = new TestIso_7_2_7();
    } else if (name == "iso_7_2_9") {
        testPtr = new TestIso_7_2_9();
    } else if (name == "iso_7_2_11") {
        testPtr = new TestIso_7_2_11();
    } else if (name == "iso_7_2_10") {
        testPtr = new TestIso_7_2_10();

    } else if (name == "iso_7_3_1") {
        testPtr = new TestIso_7_3_1();
    } else if (name == "iso_7_3_2") {
        testPtr = new TestIso_7_3_2();
    } else if (name == "iso_7_3_3") {
        testPtr = new TestIso_7_3_3();
    } else if (name == "iso_7_3_4") {
        testPtr = new TestIso_7_3_4();

    } else if (name == "iso_7_4_1") {
        testPtr = new TestIso_7_4_1();
    } else if (name == "iso_7_4_2") {
        testPtr = new TestIso_7_4_2();
    } else if (name == "iso_7_4_4") {
        testPtr = new TestIso_7_4_4();
    } else if (name == "iso_7_4_5") {
        testPtr = new TestIso_7_4_5();

    } else if (name == "iso_7_6_1") {
        testPtr = new TestIso_7_6_1();
    } else if (name == "iso_7_6_2") {
        testPtr = new TestIso_7_6_2();
    } else if (name == "iso_7_6_3") {
        testPtr = new TestIso_7_6_3();
    } else if (name == "iso_7_6_4") {
        testPtr = new TestIso_7_6_4();
    } else if (name == "iso_7_6_5") {
        testPtr = new TestIso_7_6_5();
    } else if (name == "iso_7_6_6") {
        testPtr = new TestIso_7_6_6();
    } else if (name == "iso_7_6_7") {
        testPtr = new TestIso_7_6_7();

    } else {
        std::cerr << "Unknown test name: " << name << std::endl;
    }

    testPtr->testName = name;

    return testPtr;
}


void testMessage(std::string message, ...)
{
    std::cout << "\033[1;92mSW test: \033[0m" << message << std::endl;
}


void testBigMessage(std::string message, ...)
{
    testMessage(std::string(80, '*'));
    testMessage(message);
    testMessage(std::string(80, '*'));
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