/**
 * TODO: License
 */

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "../vpi_lib/vpi_compliance_lib.hpp"

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

    /*
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
    */

   reset_agent_polarity_set(0);
   reset_agent_assert();
   reset_agent_deassert();

   std::chrono::nanoseconds clkPer(10);
   mem_bus_agent_set_period(clkPer);
   mem_bus_agent_start();
   mem_bus_agent_write32(16, 0xAABBCCDD);
   uint8_t A;
   A = mem_bus_agent_read8(0);
   printf("%x\n", A);
   A = mem_bus_agent_read8(1);
   printf("%x\n", A);
   A = mem_bus_agent_read8(2);
   printf("%x\n", A);
   A = mem_bus_agent_read8(3);
   printf("%x\n", A);

   uint16_t B = mem_bus_agent_read16(0);
   printf("%x\n", B);

   test_message("TestDemo: Run Exiting");

   return 0;
}
