/**
 * TODO: License
 */

#include <iostream>
#include <unistd.h>

extern "C" {
    #include "../vpi_lib/vpi_compliance_lib.h"
}

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../compliance_tests_includes/TestDemo.h"

test_lib::TestDemo::TestDemo() : TestBase()
{
    
};


int test_lib::TestDemo::run()
{
    TestBase::run();

    test_message("TestDemo: Run Entered");
    int polarity = 1;
    int polarity_read = 0;

    for (int i = 0; i < 10; i++)
    {
        if (polarity == 0)
            polarity = 1;
        else
            polarity = 0;

        reset_agent_polarity_set(polarity);
        polarity_read = reset_agent_polarity_get();
        //test_message("Polarity read:");
        //test_message();
    }

    test_message("TestDemo: Run Exiting");

    return 0;
}
