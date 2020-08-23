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
             * Here initialize test specific variables, create test specific
             * objects. Each test shall initialize 'elem_tests' list and fill
             * 'test_variants'. This method will be called if
             * 'SetupTestEnvironment' is called from "Run" method.
             */
        }

        /**
         * Test execution function shall consist of following actions:
         *  1. Call of "SetupTestEnvironment"
         *  2. Iterating over "elem_tests" and executing elementary test for
         *     each variant.
         *  3. Call of "CleanupTestEnvironment" and returning what it returns
         */
        int Run()
        {
            SetupTestEnvironment();

            /****************************************************************
             * Write your test code here!
             ***************************************************************/
            
            return (int)FinishTest();
        }
};