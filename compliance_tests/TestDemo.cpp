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
#include <unistd.h>
#include <chrono>

#include "../vpi_lib/vpiComplianceLib.hpp"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"
#include "../test_lib/TestLoader.h"

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

using namespace can;

class test_lib::TestDemo : public TestBase
{
    public:

        /* Here put any test dependent declarations! */

        void ConfigureTest()
        {
            /*
             * Here initialize test specific variables, create test specific objects. Each test
             * shall initialize 'elem_tests' list and fill 'test_variants'. This method will be
             * called before test is Run. If elementary tests are not filled here, nothing run!
             * 
             * In this function, TestBase::ConfigureTest was already ran, so TB should be set up,
             * configuration should be obtain by test environment (either from digital simulation
             * or other master source), IUT should be enabled, Error Active and bus integration
             * should be finished!
             */
        }

        /**
         * Execution function for each elementary test of each test variant as filled in
         * ConfigureTest function.
         * 
         * Test result can be stored to "test_result" variable.
         * 
         * @returns 0 if test should proceed, or 1 if test execution should be aborted.
         */
        int RunElemTest(const ElementaryTest &elem_test, const TestVariant &test_variant)
        {
            /****************************************************************
             * Write your test code here!
             ***************************************************************/
            TestMessage("%d %d", test_variant, elem_test.index);

            return 0;
        }
};