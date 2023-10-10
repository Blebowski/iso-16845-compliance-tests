/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 *
 *****************************************************************************/

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "pli_lib.h"

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
        int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test,
                        [[maybe_unused]] const TestVariant &test_variant)
        {
            /****************************************************************
             * Write your test code here!
             ***************************************************************/
            TestMessage("%d %d", test_variant, elem_test.index_);

            return 0;
        }
};