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
#include <cstdarg>
#include <memory>

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
#include "../compliance_tests/TestIso_7_6_8.cpp"
#include "../compliance_tests/TestIso_7_6_11.cpp"
#include "../compliance_tests/TestIso_7_6_12.cpp"
#include "../compliance_tests/TestIso_7_6_13.cpp"
#include "../compliance_tests/TestIso_7_6_14.cpp"
#include "../compliance_tests/TestIso_7_6_15.cpp"
#include "../compliance_tests/TestIso_7_6_16.cpp"
#include "../compliance_tests/TestIso_7_6_17.cpp"
#include "../compliance_tests/TestIso_7_6_18.cpp"
#include "../compliance_tests/TestIso_7_6_19.cpp"
#include "../compliance_tests/TestIso_7_6_20.cpp"

#include "../compliance_tests/TestIso_7_7_1.cpp"
#include "../compliance_tests/TestIso_7_7_3.cpp"
#include "../compliance_tests/TestIso_7_7_4.cpp"
#include "../compliance_tests/TestIso_7_7_5.cpp"
#include "../compliance_tests/TestIso_7_7_6.cpp"
#include "../compliance_tests/TestIso_7_7_7.cpp"
#include "../compliance_tests/TestIso_7_7_8.cpp"
#include "../compliance_tests/TestIso_7_7_9_1.cpp"
#include "../compliance_tests/TestIso_7_7_9_2.cpp"
#include "../compliance_tests/TestIso_7_7_10.cpp"
#include "../compliance_tests/TestIso_7_7_11.cpp"

#include "../compliance_tests/TestIso_7_8_1_1.cpp"
#include "../compliance_tests/TestIso_7_8_1_2.cpp"
#include "../compliance_tests/TestIso_7_8_1_3.cpp"
#include "../compliance_tests/TestIso_7_8_2_1.cpp"
#include "../compliance_tests/TestIso_7_8_2_2.cpp"
#include "../compliance_tests/TestIso_7_8_3_1.cpp"
#include "../compliance_tests/TestIso_7_8_3_2.cpp"
#include "../compliance_tests/TestIso_7_8_3_3.cpp"
#include "../compliance_tests/TestIso_7_8_4_1.cpp"
#include "../compliance_tests/TestIso_7_8_4_2.cpp"
#include "../compliance_tests/TestIso_7_8_4_3.cpp"
#include "../compliance_tests/TestIso_7_8_5_1.cpp"
#include "../compliance_tests/TestIso_7_8_5_2.cpp"
#include "../compliance_tests/TestIso_7_8_5_3.cpp"
#include "../compliance_tests/TestIso_7_8_6_1.cpp"
#include "../compliance_tests/TestIso_7_8_6_2.cpp"
#include "../compliance_tests/TestIso_7_8_6_3.cpp"
#include "../compliance_tests/TestIso_7_8_7_1.cpp"
#include "../compliance_tests/TestIso_7_8_7_2.cpp"
#include "../compliance_tests/TestIso_7_8_7_3.cpp"
#include "../compliance_tests/TestIso_7_8_8_1.cpp"
#include "../compliance_tests/TestIso_7_8_8_2.cpp"
#include "../compliance_tests/TestIso_7_8_8_3.cpp"
#include "../compliance_tests/TestIso_7_8_9_1.cpp"
#include "../compliance_tests/TestIso_7_8_9_2.cpp"
#include "../compliance_tests/TestIso_7_8_9_3.cpp"

#include "../compliance_tests/TestIso_8_1_1.cpp"
#include "../compliance_tests/TestIso_8_1_3.cpp"
#include "../compliance_tests/TestIso_8_1_5.cpp"
#include "../compliance_tests/TestIso_8_1_6.cpp"
#include "../compliance_tests/TestIso_8_1_7.cpp"
#include "../compliance_tests/TestIso_8_1_8.cpp"

#include "../compliance_tests/TestIso_8_2_6.cpp"
#include "../compliance_tests/TestIso_8_2_7.cpp"

#include "../compliance_tests/TestIso_8_3_1.cpp"
#include "../compliance_tests/TestIso_8_3_2.cpp"
#include "../compliance_tests/TestIso_8_3_3.cpp"
#include "../compliance_tests/TestIso_8_3_4.cpp"

#include "../compliance_tests/TestIso_8_4_1.cpp"
#include "../compliance_tests/TestIso_8_4_2.cpp"
#include "../compliance_tests/TestIso_8_4_3.cpp"
#include "../compliance_tests/TestIso_8_4_4.cpp"
#include "../compliance_tests/TestIso_8_4_5.cpp"

#include "../compliance_tests/TestIso_8_5_1.cpp"
#include "../compliance_tests/TestIso_8_5_2.cpp"
#include "../compliance_tests/TestIso_8_5_3.cpp"
#include "../compliance_tests/TestIso_8_5_4.cpp"
#include "../compliance_tests/TestIso_8_5_5.cpp"
#include "../compliance_tests/TestIso_8_5_6.cpp"
#include "../compliance_tests/TestIso_8_5_7.cpp"
#include "../compliance_tests/TestIso_8_5_8.cpp"


/******************************************************************************
 *****************************************************************************/

std::thread *testThread;


/******************************************************************************
 * Mapping of test name to Class representing the test.
 * This is simple workaround so that we don't have to implement some form
 * of reflection/factory solution which would convert us string to constructor
 *****************************************************************************/
std::unique_ptr<test_lib::TestBase> ConstructTestObject(std::string name)
{
    std::unique_ptr<test_lib::TestBase> test_ptr = NULL;

    if (name == "base") {
        test_ptr = std::make_unique<test_lib::TestBase>();
    } else if (name == "demo") {
        test_ptr = std::make_unique<test_lib::TestDemo>();
    } else if (name == "iso_7_1_1") {
        test_ptr = std::make_unique<TestIso_7_1_1>();
    } else if (name == "iso_7_1_4") {
        test_ptr = std::make_unique<TestIso_7_1_4>();
    } else if (name == "iso_7_1_5") {
        test_ptr = std::make_unique<TestIso_7_1_5>();
    } else if (name == "iso_7_1_8") {
        test_ptr = std::make_unique<TestIso_7_1_8>();
    } else if (name == "iso_7_1_9") {
        test_ptr = std::make_unique<TestIso_7_1_9>();
    } else if (name == "iso_7_1_12") {
        test_ptr = std::make_unique<TestIso_7_1_12>();
    
    } else if (name == "iso_7_2_1") {
        test_ptr = std::make_unique<TestIso_7_2_1>();
    } else if (name == "iso_7_2_6") {
        test_ptr = std::make_unique<TestIso_7_2_6>();
    } else if (name == "iso_7_2_7") {
        test_ptr = std::make_unique<TestIso_7_2_7>();
    } else if (name == "iso_7_2_9") {
        test_ptr = std::make_unique<TestIso_7_2_9>();
    } else if (name == "iso_7_2_11") {
        test_ptr = std::make_unique<TestIso_7_2_11>();
    } else if (name == "iso_7_2_10") {
        test_ptr = std::make_unique<TestIso_7_2_10>();

    } else if (name == "iso_7_3_1") {
        test_ptr = std::make_unique<TestIso_7_3_1>();
    } else if (name == "iso_7_3_2") {
        test_ptr = std::make_unique<TestIso_7_3_2>();
    } else if (name == "iso_7_3_3") {
        test_ptr = std::make_unique<TestIso_7_3_3>();
    } else if (name == "iso_7_3_4") {
        test_ptr = std::make_unique<TestIso_7_3_4>();

    } else if (name == "iso_7_4_1") {
        test_ptr = std::make_unique<TestIso_7_4_1>();
    } else if (name == "iso_7_4_2") {
        test_ptr = std::make_unique<TestIso_7_4_2>();
    } else if (name == "iso_7_4_4") {
        test_ptr = std::make_unique<TestIso_7_4_4>();
    } else if (name == "iso_7_4_5") {
        test_ptr = std::make_unique<TestIso_7_4_5>();

    } else if (name == "iso_7_6_1") {
        test_ptr = std::make_unique<TestIso_7_6_1>();
    } else if (name == "iso_7_6_2") {
        test_ptr = std::make_unique<TestIso_7_6_2>();
    } else if (name == "iso_7_6_3") {
        test_ptr = std::make_unique<TestIso_7_6_3>();
    } else if (name == "iso_7_6_4") {
        test_ptr = std::make_unique<TestIso_7_6_4>();
    } else if (name == "iso_7_6_5") {
        test_ptr = std::make_unique<TestIso_7_6_5>();
    } else if (name == "iso_7_6_6") {
        test_ptr = std::make_unique<TestIso_7_6_6>();
    } else if (name == "iso_7_6_7") {
        test_ptr = std::make_unique<TestIso_7_6_7>();
    } else if (name == "iso_7_6_8") {
        test_ptr = std::make_unique<TestIso_7_6_8>();
    } else if (name == "iso_7_6_11") {
        test_ptr = std::make_unique<TestIso_7_6_11>();
    } else if (name == "iso_7_6_12") {
        test_ptr = std::make_unique<TestIso_7_6_12>();
    } else if (name == "iso_7_6_13") {
        test_ptr = std::make_unique<TestIso_7_6_13>();
    } else if (name == "iso_7_6_14") {
        test_ptr = std::make_unique<TestIso_7_6_14>();
    } else if (name == "iso_7_6_15") {
        test_ptr = std::make_unique<TestIso_7_6_15>();
    } else if (name == "iso_7_6_16") {
        test_ptr = std::make_unique<TestIso_7_6_16>();
    } else if (name == "iso_7_6_17") {
        test_ptr = std::make_unique<TestIso_7_6_17>();
    } else if (name == "iso_7_6_18") {
        test_ptr = std::make_unique<TestIso_7_6_18>();
    } else if (name == "iso_7_6_19") {
        test_ptr = std::make_unique<TestIso_7_6_19>();
    } else if (name == "iso_7_6_20") {
        test_ptr = std::make_unique<TestIso_7_6_20>();

    } else if (name == "iso_7_7_1") {
        test_ptr = std::make_unique<TestIso_7_7_1>();
    } else if (name == "iso_7_7_3") {
        test_ptr = std::make_unique<TestIso_7_7_3>();
    } else if (name == "iso_7_7_4") {
        test_ptr = std::make_unique<TestIso_7_7_4>();
    } else if (name == "iso_7_7_5") {
        test_ptr = std::make_unique<TestIso_7_7_5>();
    } else if (name == "iso_7_7_6") {
        test_ptr = std::make_unique<TestIso_7_7_6>();
    } else if (name == "iso_7_7_7") {
        test_ptr = std::make_unique<TestIso_7_7_7>();
    } else if (name == "iso_7_7_8") {
        test_ptr = std::make_unique<TestIso_7_7_8>();
    } else if (name == "iso_7_7_9_1") {
        test_ptr = std::make_unique<TestIso_7_7_9_1>();
    } else if (name == "iso_7_7_9_2") {
        test_ptr = std::make_unique<TestIso_7_7_9_2>();
    } else if (name == "iso_7_7_10") {
        test_ptr = std::make_unique<TestIso_7_7_10>();
    } else if (name == "iso_7_7_11") {
        test_ptr = std::make_unique<TestIso_7_7_11>();

    } else if (name == "iso_7_8_1_1") {
        test_ptr = std::make_unique<TestIso_7_8_1_1>();
    } else if (name == "iso_7_8_1_2") {
        test_ptr = std::make_unique<TestIso_7_8_1_2>();
    } else if (name == "iso_7_8_1_3") {
        test_ptr = std::make_unique<TestIso_7_8_1_3>();
    } else if (name == "iso_7_8_2_1") {
        test_ptr = std::make_unique<TestIso_7_8_2_1>();
    } else if (name == "iso_7_8_2_2") {
        test_ptr = std::make_unique<TestIso_7_8_2_2>();
    } else if (name == "iso_7_8_3_1") {
        test_ptr = std::make_unique<TestIso_7_8_3_1>();
    } else if (name == "iso_7_8_3_2") {
        test_ptr = std::make_unique<TestIso_7_8_3_2>();
    } else if (name == "iso_7_8_3_3") {
        test_ptr = std::make_unique<TestIso_7_8_3_3>();
    } else if (name == "iso_7_8_4_1") {
        test_ptr = std::make_unique<TestIso_7_8_4_1>();
    } else if (name == "iso_7_8_4_2") {
        test_ptr = std::make_unique<TestIso_7_8_4_2>();
    } else if (name == "iso_7_8_4_3") {
        test_ptr = std::make_unique<TestIso_7_8_4_3>();
    } else if (name == "iso_7_8_5_1") {
        test_ptr = std::make_unique<TestIso_7_8_5_1>();
    } else if (name == "iso_7_8_5_2") {
        test_ptr = std::make_unique<TestIso_7_8_5_2>();
    } else if (name == "iso_7_8_5_3") {
        test_ptr = std::make_unique<TestIso_7_8_5_3>();
    } else if (name == "iso_7_8_6_1") {
        test_ptr = std::make_unique<TestIso_7_8_6_1>();
    } else if (name == "iso_7_8_6_2") {
        test_ptr = std::make_unique<TestIso_7_8_6_2>();
    } else if (name == "iso_7_8_6_3") {
        test_ptr = std::make_unique<TestIso_7_8_6_3>();
    } else if (name == "iso_7_8_7_1") {
        test_ptr = std::make_unique<TestIso_7_8_7_1>();
    } else if (name == "iso_7_8_7_2") {
        test_ptr = std::make_unique<TestIso_7_8_7_2>();
    } else if (name == "iso_7_8_7_3") {
        test_ptr = std::make_unique<TestIso_7_8_7_3>();
    } else if (name == "iso_7_8_8_1") {
        test_ptr = std::make_unique<TestIso_7_8_8_1>();
    } else if (name == "iso_7_8_8_2") {
        test_ptr = std::make_unique<TestIso_7_8_8_2>();
    } else if (name == "iso_7_8_8_3") {
        test_ptr = std::make_unique<TestIso_7_8_8_3>();
    } else if (name == "iso_7_8_9_1") {
        test_ptr = std::make_unique<TestIso_7_8_9_1>();
    } else if (name == "iso_7_8_9_2") {
        test_ptr = std::make_unique<TestIso_7_8_9_2>();
    } else if (name == "iso_7_8_9_3") {
        test_ptr = std::make_unique<TestIso_7_8_9_3>();

    } else if (name == "iso_8_1_1") {
        test_ptr = std::make_unique<TestIso_8_1_1>();
    } else if (name == "iso_8_1_3") {
        test_ptr = std::make_unique<TestIso_8_1_3>();
    } else if (name == "iso_8_1_5") {
        test_ptr = std::make_unique<TestIso_8_1_5>();
    } else if (name == "iso_8_1_6") {
        test_ptr = std::make_unique<TestIso_8_1_6>();
    } else if (name == "iso_8_1_7") {
        test_ptr = std::make_unique<TestIso_8_1_7>();
    } else if (name == "iso_8_1_8") {
        test_ptr = std::make_unique<TestIso_8_1_8>();
    } else if (name == "iso_8_2_6") {
        test_ptr = std::make_unique<TestIso_8_2_6>();
    } else if (name == "iso_8_2_7") {
        test_ptr = std::make_unique<TestIso_8_2_7>();

    } else if (name == "iso_8_3_1") {
        test_ptr = std::make_unique<TestIso_8_3_1>();
    } else if (name == "iso_8_3_2") {
        test_ptr = std::make_unique<TestIso_8_3_2>();
    } else if (name == "iso_8_3_3") {
        test_ptr = std::make_unique<TestIso_8_3_3>();
    } else if (name == "iso_8_3_4") {
        test_ptr = std::make_unique<TestIso_8_3_4>();

    } else if (name == "iso_8_4_1") {
        test_ptr = std::make_unique<TestIso_8_4_1>();
    } else if (name == "iso_8_4_2") {
        test_ptr = std::make_unique<TestIso_8_4_2>();
    } else if (name == "iso_8_4_3") {
        test_ptr = std::make_unique<TestIso_8_4_3>();
    } else if (name == "iso_8_4_4") {
        test_ptr = std::make_unique<TestIso_8_4_4>();
    } else if (name == "iso_8_4_5") {
        test_ptr = std::make_unique<TestIso_8_4_5>();

    } else if (name == "iso_8_5_1") {
        test_ptr = std::make_unique<TestIso_8_5_1>();
    } else if (name == "iso_8_5_2") {
        test_ptr = std::make_unique<TestIso_8_5_2>();
    } else if (name == "iso_8_5_3") {
        test_ptr = std::make_unique<TestIso_8_5_3>();
    } else if (name == "iso_8_5_4") {
        test_ptr = std::make_unique<TestIso_8_5_4>();
    } else if (name == "iso_8_5_5") {
        test_ptr = std::make_unique<TestIso_8_5_5>();
    } else if (name == "iso_8_5_6") {
        test_ptr = std::make_unique<TestIso_8_5_6>();
    } else if (name == "iso_8_5_7") {
        test_ptr = std::make_unique<TestIso_8_5_7>();
    } else if (name == "iso_8_5_8") {
        test_ptr = std::make_unique<TestIso_8_5_8>();

    } else {
        std::cerr << "Unknown test name: " << name << std::endl;
    }

    test_ptr->test_name = name;

    return std::move(test_ptr);
}


void TestMessage(std::string message, ...)
{
    std::cout << "\033[1;92mSW test: \033[0m" << message << std::endl;
}


void TestBigMessage(std::string message, ...)
{
    TestMessage(std::string(80, '*'));
    TestMessage(message);
    TestMessage(std::string(80, '*'));
}


int cppTestThread(char *test_name)
{   
    std::unique_ptr<test_lib::TestBase> cppTest = ConstructTestObject(test_name);
    return cppTest->Run();
}


void RunCppTest(char* test_name)
{
    TestMessage(std::string(80, '*'));
    TestMessage("Running C++ test: %s", test_name);
    TestMessage(std::string(80, '*'));

    testThread = new std::thread(cppTestThread, test_name);

    // TODO: Destroy thread object when it ends! We can't join here since this
    //       is called in simulator context and must end before the simulator
    //       proceeds with next simulation time. That would cause deadlock!
}