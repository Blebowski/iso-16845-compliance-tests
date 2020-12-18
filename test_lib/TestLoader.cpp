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
#include "../compliance_tests/TestIso_7_1_2.cpp"
#include "../compliance_tests/TestIso_7_1_3.cpp"
#include "../compliance_tests/TestIso_7_1_4.cpp"
#include "../compliance_tests/TestIso_7_1_5.cpp"
#include "../compliance_tests/TestIso_7_1_6.cpp"
#include "../compliance_tests/TestIso_7_1_7.cpp"
#include "../compliance_tests/TestIso_7_1_8.cpp"
#include "../compliance_tests/TestIso_7_1_9.cpp"
#include "../compliance_tests/TestIso_7_1_10.cpp"
#include "../compliance_tests/TestIso_7_1_11.cpp"
#include "../compliance_tests/TestIso_7_1_12.cpp"

#include "../compliance_tests/TestIso_7_2_1.cpp"
#include "../compliance_tests/TestIso_7_2_2.cpp"
#include "../compliance_tests/TestIso_7_2_3.cpp"
#include "../compliance_tests/TestIso_7_2_4.cpp"
#include "../compliance_tests/TestIso_7_2_5.cpp"
#include "../compliance_tests/TestIso_7_2_6.cpp"
#include "../compliance_tests/TestIso_7_2_7.cpp"
#include "../compliance_tests/TestIso_7_2_8.cpp"
#include "../compliance_tests/TestIso_7_2_9.cpp"
#include "../compliance_tests/TestIso_7_2_10.cpp"
#include "../compliance_tests/TestIso_7_2_11.cpp"

#include "../compliance_tests/TestIso_7_3_1.cpp"
#include "../compliance_tests/TestIso_7_3_2.cpp"
#include "../compliance_tests/TestIso_7_3_3.cpp"
#include "../compliance_tests/TestIso_7_3_4.cpp"

#include "../compliance_tests/TestIso_7_4_1.cpp"
#include "../compliance_tests/TestIso_7_4_2.cpp"
#include "../compliance_tests/TestIso_7_4_3.cpp"
#include "../compliance_tests/TestIso_7_4_4.cpp"
#include "../compliance_tests/TestIso_7_4_5.cpp"
#include "../compliance_tests/TestIso_7_4_6.cpp"
#include "../compliance_tests/TestIso_7_4_7.cpp"

#include "../compliance_tests/TestIso_7_5_1.cpp"
#include "../compliance_tests/TestIso_7_5_2.cpp"
#include "../compliance_tests/TestIso_7_5_3.cpp"
#include "../compliance_tests/TestIso_7_5_4.cpp"
#include "../compliance_tests/TestIso_7_5_5.cpp"
#include "../compliance_tests/TestIso_7_5_6.cpp"
#include "../compliance_tests/TestIso_7_5_7.cpp"

#include "../compliance_tests/TestIso_7_6_1.cpp"
#include "../compliance_tests/TestIso_7_6_2.cpp"
#include "../compliance_tests/TestIso_7_6_3.cpp"
#include "../compliance_tests/TestIso_7_6_4.cpp"
#include "../compliance_tests/TestIso_7_6_5.cpp"
#include "../compliance_tests/TestIso_7_6_6.cpp"
#include "../compliance_tests/TestIso_7_6_7.cpp"
#include "../compliance_tests/TestIso_7_6_8.cpp"
#include "../compliance_tests/TestIso_7_6_9.cpp"
#include "../compliance_tests/TestIso_7_6_10.cpp"
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
#include "../compliance_tests/TestIso_7_6_21.cpp"
#include "../compliance_tests/TestIso_7_6_22.cpp"
#include "../compliance_tests/TestIso_7_6_23.cpp"

#include "../compliance_tests/TestIso_7_7_1.cpp"
#include "../compliance_tests/TestIso_7_7_2.cpp"
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
#include "../compliance_tests/TestIso_8_1_2.cpp"
#include "../compliance_tests/TestIso_8_1_3.cpp"
#include "../compliance_tests/TestIso_8_1_4.cpp"
#include "../compliance_tests/TestIso_8_1_5.cpp"
#include "../compliance_tests/TestIso_8_1_6.cpp"
#include "../compliance_tests/TestIso_8_1_7.cpp"
#include "../compliance_tests/TestIso_8_1_8.cpp"

#include "../compliance_tests/TestIso_8_2_1.cpp"
#include "../compliance_tests/TestIso_8_2_2.cpp"
#include "../compliance_tests/TestIso_8_2_3.cpp"
#include "../compliance_tests/TestIso_8_2_4.cpp"
#include "../compliance_tests/TestIso_8_2_5.cpp"
#include "../compliance_tests/TestIso_8_2_6.cpp"
#include "../compliance_tests/TestIso_8_2_7.cpp"
#include "../compliance_tests/TestIso_8_2_8.cpp"

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
#include "../compliance_tests/TestIso_8_5_9.cpp"
#include "../compliance_tests/TestIso_8_5_10.cpp"
#include "../compliance_tests/TestIso_8_5_11.cpp"
#include "../compliance_tests/TestIso_8_5_12.cpp"
#include "../compliance_tests/TestIso_8_5_13.cpp"
#include "../compliance_tests/TestIso_8_5_14.cpp"
#include "../compliance_tests/TestIso_8_5_15.cpp"

#include "../compliance_tests/TestIso_8_6_1.cpp"
#include "../compliance_tests/TestIso_8_6_2.cpp"
#include "../compliance_tests/TestIso_8_6_3.cpp"
#include "../compliance_tests/TestIso_8_6_4.cpp"
#include "../compliance_tests/TestIso_8_6_5.cpp"
#include "../compliance_tests/TestIso_8_6_6.cpp"
#include "../compliance_tests/TestIso_8_6_7.cpp"
#include "../compliance_tests/TestIso_8_6_8.cpp"
#include "../compliance_tests/TestIso_8_6_9.cpp"
#include "../compliance_tests/TestIso_8_6_10.cpp"
#include "../compliance_tests/TestIso_8_6_11.cpp"
#include "../compliance_tests/TestIso_8_6_12.cpp"
#include "../compliance_tests/TestIso_8_6_13.cpp"
#include "../compliance_tests/TestIso_8_6_14.cpp"
#include "../compliance_tests/TestIso_8_6_15.cpp"
#include "../compliance_tests/TestIso_8_6_16.cpp"
#include "../compliance_tests/TestIso_8_6_17.cpp"
#include "../compliance_tests/TestIso_8_6_18.cpp"
#include "../compliance_tests/TestIso_8_6_19.cpp"
#include "../compliance_tests/TestIso_8_6_20.cpp"
#include "../compliance_tests/TestIso_8_6_21.cpp"

#include "../compliance_tests/TestIso_8_7_1.cpp"
#include "../compliance_tests/TestIso_8_7_2.cpp"
#include "../compliance_tests/TestIso_8_7_3.cpp"
#include "../compliance_tests/TestIso_8_7_4.cpp"
#include "../compliance_tests/TestIso_8_7_5.cpp"
#include "../compliance_tests/TestIso_8_7_6.cpp"
#include "../compliance_tests/TestIso_8_7_7.cpp"
#include "../compliance_tests/TestIso_8_7_8.cpp"
#include "../compliance_tests/TestIso_8_7_9.cpp"

#include "../compliance_tests/TestIso_8_8_1_1.cpp"
#include "../compliance_tests/TestIso_8_8_1_2.cpp"
#include "../compliance_tests/TestIso_8_8_1_3.cpp"


/******************************************************************************
 *****************************************************************************/

std::thread *testThread;


/******************************************************************************
 * Mapping of test name to Class representing the test.
 * This is simple workaround so that we don't have to implement some form
 * of reflection/factory solution which would convert us string to constructor
 *****************************************************************************/
test_lib::TestBase* ConstructTestObject(std::string name)
{
    test_lib::TestBase *test_ptr = NULL;

    if (name == "base") {
        test_ptr = new TestBase;
    } else if (name == "demo") {
        test_ptr = new TestDemo;
    } else if (name == "iso_7_1_1") {
        test_ptr = new TestIso_7_1_1;
    } else if (name == "iso_7_1_2") {
        test_ptr = new TestIso_7_1_2;
    } else if (name == "iso_7_1_3") {
        test_ptr = new TestIso_7_1_3;
    } else if (name == "iso_7_1_4") {
        test_ptr = new TestIso_7_1_4;
    } else if (name == "iso_7_1_5") {
        test_ptr = new TestIso_7_1_5;
    } else if (name == "iso_7_1_6") {
        test_ptr = new TestIso_7_1_6;
    } else if (name == "iso_7_1_7") {
        test_ptr = new TestIso_7_1_7;
    } else if (name == "iso_7_1_8") {
        test_ptr = new TestIso_7_1_8;
    } else if (name == "iso_7_1_9") {
        test_ptr = new TestIso_7_1_9;
    } else if (name == "iso_7_1_10") {
        test_ptr = new TestIso_7_1_10;
    } else if (name == "iso_7_1_11") {
        test_ptr = new TestIso_7_1_11;
    } else if (name == "iso_7_1_12") {
        test_ptr = new TestIso_7_1_12;
    
    } else if (name == "iso_7_2_1") {
        test_ptr = new TestIso_7_2_1;
    } else if (name == "iso_7_2_2") {
        test_ptr = new TestIso_7_2_2;
    } else if (name == "iso_7_2_3") {
        test_ptr = new TestIso_7_2_3;
    } else if (name == "iso_7_2_4") {
        test_ptr = new TestIso_7_2_4;
    } else if (name == "iso_7_2_5") {
        test_ptr = new TestIso_7_2_5;
    } else if (name == "iso_7_2_6") {
        test_ptr = new TestIso_7_2_6;
    } else if (name == "iso_7_2_7") {
        test_ptr = new TestIso_7_2_7;
    } else if (name == "iso_7_2_8") {
        test_ptr = new TestIso_7_2_8;
    } else if (name == "iso_7_2_9") {
        test_ptr = new TestIso_7_2_9;
    } else if (name == "iso_7_2_11") {
        test_ptr = new TestIso_7_2_11;
    } else if (name == "iso_7_2_10") {
        test_ptr = new TestIso_7_2_10;

    } else if (name == "iso_7_3_1") {
        test_ptr = new TestIso_7_3_1;
    } else if (name == "iso_7_3_2") {
        test_ptr = new TestIso_7_3_2;
    } else if (name == "iso_7_3_3") {
        test_ptr = new TestIso_7_3_3;
    } else if (name == "iso_7_3_4") {
        test_ptr = new TestIso_7_3_4;

    } else if (name == "iso_7_4_1") {
        test_ptr = new TestIso_7_4_1;
    } else if (name == "iso_7_4_2") {
        test_ptr = new TestIso_7_4_2;
    } else if (name == "iso_7_4_3") {
        test_ptr = new TestIso_7_4_3;
    } else if (name == "iso_7_4_4") {
        test_ptr = new TestIso_7_4_4;
    } else if (name == "iso_7_4_5") {
        test_ptr = new TestIso_7_4_5;
    } else if (name == "iso_7_4_6") {
        test_ptr = new TestIso_7_4_6;
    } else if (name == "iso_7_4_7") {
        test_ptr = new TestIso_7_4_7;

    } else if (name == "iso_7_5_1") {
        test_ptr = new TestIso_7_5_1;
    } else if (name == "iso_7_5_2") {
        test_ptr = new TestIso_7_5_2;
    } else if (name == "iso_7_5_3") {
        test_ptr = new TestIso_7_5_3;
    } else if (name == "iso_7_5_4") {
        test_ptr = new TestIso_7_5_4;
    } else if (name == "iso_7_5_5") {
        test_ptr = new TestIso_7_5_5;
    } else if (name == "iso_7_5_6") {
        test_ptr = new TestIso_7_5_6;
    } else if (name == "iso_7_5_7") {
        test_ptr = new TestIso_7_5_7;

    } else if (name == "iso_7_6_1") {
        test_ptr = new TestIso_7_6_1;
    } else if (name == "iso_7_6_2") {
        test_ptr = new TestIso_7_6_2;
    } else if (name == "iso_7_6_3") {
        test_ptr = new TestIso_7_6_3;
    } else if (name == "iso_7_6_4") {
        test_ptr = new TestIso_7_6_4;
    } else if (name == "iso_7_6_5") {
        test_ptr = new TestIso_7_6_5;
    } else if (name == "iso_7_6_6") {
        test_ptr = new TestIso_7_6_6;
    } else if (name == "iso_7_6_7") {
        test_ptr = new TestIso_7_6_7;
    } else if (name == "iso_7_6_8") {
        test_ptr = new TestIso_7_6_8;
    } else if (name == "iso_7_6_9") {
        test_ptr = new TestIso_7_6_9;
    } else if (name == "iso_7_6_10") {
        test_ptr = new TestIso_7_6_10;
    } else if (name == "iso_7_6_11") {
        test_ptr = new TestIso_7_6_11;
    } else if (name == "iso_7_6_12") {
        test_ptr = new TestIso_7_6_12;
    } else if (name == "iso_7_6_13") {
        test_ptr = new TestIso_7_6_13;
    } else if (name == "iso_7_6_14") {
        test_ptr = new TestIso_7_6_14;
    } else if (name == "iso_7_6_15") {
        test_ptr = new TestIso_7_6_15;
    } else if (name == "iso_7_6_16") {
        test_ptr = new TestIso_7_6_16;
    } else if (name == "iso_7_6_17") {
        test_ptr = new TestIso_7_6_17;
    } else if (name == "iso_7_6_18") {
        test_ptr = new TestIso_7_6_18;
    } else if (name == "iso_7_6_19") {
        test_ptr = new TestIso_7_6_19;
    } else if (name == "iso_7_6_20") {
        test_ptr = new TestIso_7_6_20;
    } else if (name == "iso_7_6_21") {
        test_ptr = new TestIso_7_6_21;
    } else if (name == "iso_7_6_22") {
        test_ptr = new TestIso_7_6_22;
    } else if (name == "iso_7_6_23") {
        test_ptr = new TestIso_7_6_23;

    } else if (name == "iso_7_7_1") {
        test_ptr = new TestIso_7_7_1;
    } else if (name == "iso_7_7_2") {
        test_ptr = new TestIso_7_7_2;
    } else if (name == "iso_7_7_3") {
        test_ptr = new TestIso_7_7_3;
    } else if (name == "iso_7_7_4") {
        test_ptr = new TestIso_7_7_4;
    } else if (name == "iso_7_7_5") {
        test_ptr = new TestIso_7_7_5;
    } else if (name == "iso_7_7_6") {
        test_ptr = new TestIso_7_7_6;
    } else if (name == "iso_7_7_7") {
        test_ptr = new TestIso_7_7_7;
    } else if (name == "iso_7_7_8") {
        test_ptr = new TestIso_7_7_8;
    } else if (name == "iso_7_7_9_1") {
        test_ptr = new TestIso_7_7_9_1;
    } else if (name == "iso_7_7_9_2") {
        test_ptr = new TestIso_7_7_9_2;
    } else if (name == "iso_7_7_10") {
        test_ptr = new TestIso_7_7_10;
    } else if (name == "iso_7_7_11") {
        test_ptr = new TestIso_7_7_11;

    } else if (name == "iso_7_8_1_1") {
        test_ptr = new TestIso_7_8_1_1;
    } else if (name == "iso_7_8_1_2") {
        test_ptr = new TestIso_7_8_1_2;
    } else if (name == "iso_7_8_1_3") {
        test_ptr = new TestIso_7_8_1_3;
    } else if (name == "iso_7_8_2_1") {
        test_ptr = new TestIso_7_8_2_1;
    } else if (name == "iso_7_8_2_2") {
        test_ptr = new TestIso_7_8_2_2;
    } else if (name == "iso_7_8_3_1") {
        test_ptr = new TestIso_7_8_3_1;
    } else if (name == "iso_7_8_3_2") {
        test_ptr = new TestIso_7_8_3_2;
    } else if (name == "iso_7_8_3_3") {
        test_ptr = new TestIso_7_8_3_3;
    } else if (name == "iso_7_8_4_1") {
        test_ptr = new TestIso_7_8_4_1;
    } else if (name == "iso_7_8_4_2") {
        test_ptr = new TestIso_7_8_4_2;
    } else if (name == "iso_7_8_4_3") {
        test_ptr = new TestIso_7_8_4_3;
    } else if (name == "iso_7_8_5_1") {
        test_ptr = new TestIso_7_8_5_1;
    } else if (name == "iso_7_8_5_2") {
        test_ptr = new TestIso_7_8_5_2;
    } else if (name == "iso_7_8_5_3") {
        test_ptr = new TestIso_7_8_5_3;
    } else if (name == "iso_7_8_6_1") {
        test_ptr = new TestIso_7_8_6_1;
    } else if (name == "iso_7_8_6_2") {
        test_ptr = new TestIso_7_8_6_2;
    } else if (name == "iso_7_8_6_3") {
        test_ptr = new TestIso_7_8_6_3;
    } else if (name == "iso_7_8_7_1") {
        test_ptr = new TestIso_7_8_7_1;
    } else if (name == "iso_7_8_7_2") {
        test_ptr = new TestIso_7_8_7_2;
    } else if (name == "iso_7_8_7_3") {
        test_ptr = new TestIso_7_8_7_3;
    } else if (name == "iso_7_8_8_1") {
        test_ptr = new TestIso_7_8_8_1;
    } else if (name == "iso_7_8_8_2") {
        test_ptr = new TestIso_7_8_8_2;
    } else if (name == "iso_7_8_8_3") {
        test_ptr = new TestIso_7_8_8_3;
    } else if (name == "iso_7_8_9_1") {
        test_ptr = new TestIso_7_8_9_1;
    } else if (name == "iso_7_8_9_2") {
        test_ptr = new TestIso_7_8_9_2;
    } else if (name == "iso_7_8_9_3") {
        test_ptr = new TestIso_7_8_9_3;

    } else if (name == "iso_8_1_1") {
        test_ptr = new TestIso_8_1_1;
    } else if (name == "iso_8_1_2") {
        test_ptr = new TestIso_8_1_2;
    } else if (name == "iso_8_1_3") {
        test_ptr = new TestIso_8_1_3;
    } else if (name == "iso_8_1_4") {
        test_ptr = new TestIso_8_1_4;
    } else if (name == "iso_8_1_5") {
        test_ptr = new TestIso_8_1_5;
    } else if (name == "iso_8_1_6") {
        test_ptr = new TestIso_8_1_6;
    } else if (name == "iso_8_1_7") {
        test_ptr = new TestIso_8_1_7;
    } else if (name == "iso_8_1_8") {
        test_ptr = new TestIso_8_1_8;
    
    } else if (name == "iso_8_2_1") {
        test_ptr = new TestIso_8_2_1;
    } else if (name == "iso_8_2_2") {
        test_ptr = new TestIso_8_2_2;
    } else if (name == "iso_8_2_3") {
        test_ptr = new TestIso_8_2_3;
    } else if (name == "iso_8_2_4") {
        test_ptr = new TestIso_8_2_4;
    } else if (name == "iso_8_2_5") {
        test_ptr = new TestIso_8_2_5;
    } else if (name == "iso_8_2_6") {
        test_ptr = new TestIso_8_2_6;
    } else if (name == "iso_8_2_7") {
        test_ptr = new TestIso_8_2_7;
    } else if (name == "iso_8_2_8") {
        test_ptr = new TestIso_8_2_8;

    } else if (name == "iso_8_3_1") {
        test_ptr = new TestIso_8_3_1;
    } else if (name == "iso_8_3_2") {
        test_ptr = new TestIso_8_3_2;
    } else if (name == "iso_8_3_3") {
        test_ptr = new TestIso_8_3_3;
    } else if (name == "iso_8_3_4") {
        test_ptr = new TestIso_8_3_4;

    } else if (name == "iso_8_4_1") {
        test_ptr = new TestIso_8_4_1;
    } else if (name == "iso_8_4_2") {
        test_ptr = new TestIso_8_4_2;
    } else if (name == "iso_8_4_3") {
        test_ptr = new TestIso_8_4_3;
    } else if (name == "iso_8_4_4") {
        test_ptr = new TestIso_8_4_4;
    } else if (name == "iso_8_4_5") {
        test_ptr = new TestIso_8_4_5;

    } else if (name == "iso_8_5_1") {
        test_ptr = new TestIso_8_5_1;
    } else if (name == "iso_8_5_2") {
        test_ptr = new TestIso_8_5_2;
    } else if (name == "iso_8_5_3") {
        test_ptr = new TestIso_8_5_3;
    } else if (name == "iso_8_5_4") {
        test_ptr = new TestIso_8_5_4;
    } else if (name == "iso_8_5_5") {
        test_ptr = new TestIso_8_5_5;
    } else if (name == "iso_8_5_6") {
        test_ptr = new TestIso_8_5_6;
    } else if (name == "iso_8_5_7") {
        test_ptr = new TestIso_8_5_7;
    } else if (name == "iso_8_5_8") {
        test_ptr = new TestIso_8_5_8;
    } else if (name == "iso_8_5_9") {
        test_ptr = new TestIso_8_5_9;
    } else if (name == "iso_8_5_10") {
        test_ptr = new TestIso_8_5_10;
    } else if (name == "iso_8_5_11") {
        test_ptr = new TestIso_8_5_11;
    } else if (name == "iso_8_5_12") {
        test_ptr = new TestIso_8_5_12;
    } else if (name == "iso_8_5_13") {
        test_ptr = new TestIso_8_5_13;
    } else if (name == "iso_8_5_14") {
        test_ptr = new TestIso_8_5_14;
    } else if (name == "iso_8_5_15") {
        test_ptr = new TestIso_8_5_15;

    } else if (name == "iso_8_6_1") {
        test_ptr = new TestIso_8_6_1;
    } else if (name == "iso_8_6_2") {
        test_ptr = new TestIso_8_6_2;
    } else if (name == "iso_8_6_3") {
        test_ptr = new TestIso_8_6_3;
    } else if (name == "iso_8_6_4") {
        test_ptr = new TestIso_8_6_4;
    } else if (name == "iso_8_6_5") {
        test_ptr = new TestIso_8_6_5;
    } else if (name == "iso_8_6_6") {
        test_ptr = new TestIso_8_6_6;
    } else if (name == "iso_8_6_7") {
        test_ptr = new TestIso_8_6_7;
    } else if (name == "iso_8_6_8") {
        test_ptr = new TestIso_8_6_8;
    } else if (name == "iso_8_6_9") {
        test_ptr = new TestIso_8_6_9;
    } else if (name == "iso_8_6_10") {
        test_ptr = new TestIso_8_6_10;
    } else if (name == "iso_8_6_11") {
        test_ptr = new TestIso_8_6_11;
    } else if (name == "iso_8_6_12") {
        test_ptr = new TestIso_8_6_12;
    } else if (name == "iso_8_6_13") {
        test_ptr = new TestIso_8_6_13;
    } else if (name == "iso_8_6_14") {
        test_ptr = new TestIso_8_6_14;
    } else if (name == "iso_8_6_15") {
        test_ptr = new TestIso_8_6_15;
    } else if (name == "iso_8_6_16") {
        test_ptr = new TestIso_8_6_16;
    } else if (name == "iso_8_6_17") {
        test_ptr = new TestIso_8_6_17;
    } else if (name == "iso_8_6_18") {
        test_ptr = new TestIso_8_6_18;
    } else if (name == "iso_8_6_19") {
        test_ptr = new TestIso_8_6_19;
    } else if (name == "iso_8_6_20") {
        test_ptr = new TestIso_8_6_20;
    } else if (name == "iso_8_6_21") {
        test_ptr = new TestIso_8_6_21;

    } else if (name == "iso_8_7_1") {
        test_ptr = new TestIso_8_7_1;
    } else if (name == "iso_8_7_2") {
        test_ptr = new TestIso_8_7_2;
    } else if (name == "iso_8_7_3") {
        test_ptr = new TestIso_8_7_3;
    } else if (name == "iso_8_7_4") {
        test_ptr = new TestIso_8_7_4;
    } else if (name == "iso_8_7_5") {
        test_ptr = new TestIso_8_7_5;
    } else if (name == "iso_8_7_6") {
        test_ptr = new TestIso_8_7_6;
    } else if (name == "iso_8_7_7") {
        test_ptr = new TestIso_8_7_7;
    } else if (name == "iso_8_7_8") {
        test_ptr = new TestIso_8_7_8;
    } else if (name == "iso_8_7_9") {
        test_ptr = new TestIso_8_7_9;

    } else if (name == "iso_8_8_1_1") {
        test_ptr = new TestIso_8_8_1_1;
    } else if (name == "iso_8_8_1_2") {
        test_ptr = new TestIso_8_8_1_2;
    } else if (name == "iso_8_8_1_3") {
        test_ptr = new TestIso_8_8_1_3;

    } else {
        std::cerr << "Unknown test name: " << name << std::endl;
    }

    test_ptr->test_name = name;

    return std::move(test_ptr);
}


void TestMessage(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    printf("\033[1;92mSW test: \033[0m");

    while (*fmt != '\0')
    {
        if (*fmt != '%')
        {
            printf("%c", *fmt);
            fmt++;
            continue;
        }

        fmt++;

        switch (*fmt)
        {
        case 'd':
            printf("%d", va_arg(args, int));
            break;
        case 'f':
            printf("%f", va_arg(args, double));
            break;
        case 's':
            printf("%s", va_arg(args, char *));
            break;
        case 'c':
            printf("%c", va_arg(args, int));
            break;
        default:
            break;
        }
        fmt++;
    }
    va_end(args);
    printf("\n");
}

void TestBigMessage(std::string message, ...)
{
    TestMessage(std::string(80, '*').c_str());
    TestMessage(message.c_str());
    TestMessage(std::string(80, '*').c_str());
}


int cppTestThread(char *test_name)
{   
    test_lib::TestBase *cpp_test = ConstructTestObject(test_name);
    int test_result = cpp_test->Run();
    delete cpp_test;
    return test_result;
}

void RunCppTest(char* test_name)
{
    TestMessage(std::string(80, '*').c_str());
    TestMessage("Running C++ test: %s", test_name);
    TestMessage(std::string(80, '*').c_str());

    testThread = new std::thread(cppTestThread, test_name);

    // TODO: Destroy thread object when it ends! We can't join here since this
    //       is called in simulator context and must end before the simulator
    //       proceeds with next simulation time. That would cause deadlock!
}