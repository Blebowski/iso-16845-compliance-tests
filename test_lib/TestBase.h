/**
 * TODO: License
 */

#include <chrono>

#include "test_lib.h"
#include "../can_lib/can.h"
#include "../can_lib/BitTiming.h"
#include "../can_lib/DutInterface.h"

#ifndef TEST_BASE
#define TEST_BASE

class test_lib::TestBase
{
    public:
        TestBase();

        /**
         * Test configuration.
         * 
         * Path of test configuration is like so:
         *  1. YAML config file.
         *  2. VUnit applies it to TB top generics. Generics are propagated to
         *     Test controller agent.
         *  3. Test reads this configuration from test controller agent via VPI! 
         */
        std::chrono::nanoseconds dutClockPeriod;
        can::BitTiming nominalBitTiming;
        can::BitTiming dataBitTiming;

        /**
         * DUT Interface.
         */
        can::DutInterface* dutIfc;

        virtual int run();
};

#endif