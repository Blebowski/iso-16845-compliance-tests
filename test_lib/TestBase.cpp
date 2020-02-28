/**
 * TODO: License
 */

#include <iostream>

#include "test_lib.h"
#include "TestBase.h"
#include "TestLoader.h"
#include <unistd.h>

test_lib::TestBase::TestBase()
{
    return;
}

int test_lib::TestBase::run()
{
    test_message("TestBase: Run Entered");
    usleep(10000);
    test_message("TestBase: Run Exiting");
    return 0;
}