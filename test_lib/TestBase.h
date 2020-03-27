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

#include <chrono>

#include "test_lib.h"
#include "../can_lib/can.h"
#include "../can_lib/BitTiming.h"
#include "../can_lib/DutInterface.h"

#ifndef TEST_BASE
#define TEST_BASE


/**
 * @namespace test_lib
 * @class TestBase
 * @brief Test Base class
 * 
 * Contains common attributes used by all tests.
 * 
 * @note Each test should inherit from this class.
 * @note Each test should call TestBase::run() in the beginning of its run
 *       function.
 */
class test_lib::TestBase
{
    public:

        /**
         * Each class inheriting from TestBase should call this constructor
         * before any other actions.
         */
        TestBase();

        /**********************************************************************
         * Test configuration.
         * 
         * Path of test configuration is like so:
         *  1. YAML config file.
         *  2. VUnit applies it to TB top generics. Generics are propagated to
         *     Test controller agent.
         *  3. Test reads this configuration from test controller agent via VPI! 
         **********************************************************************/

        /**
         * Clock period to be set in TB.
         */
        std::chrono::nanoseconds dutClockPeriod;

        /**
         * CAN bus Bit timing setting for nominal/data bit rate.
         */
        can::BitTiming nominalBitTiming;
        can::BitTiming dataBitTiming;

        /**
         * Test name
         */
        std::string testName;

        /**
         * Pointer to DUT Interface object. Object created when TestBase object
         * is created. Used to access DUT by tests.
         */
        can::DutInterface* dutIfc;

        /**
         * Pointer to DUT Interface object. Object created when TestBase object
         * is created. Used to access DUT by tests.
         */
        virtual int run();
};

#endif