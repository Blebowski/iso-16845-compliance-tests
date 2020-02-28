/**
 * TODO: License
 */

#include "test_lib.h"

#ifndef TEST_BASE
#define TEST_BASE

class test_lib::TestBase
{
    public:
        TestBase();

        virtual int run();
};

#endif