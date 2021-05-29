/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 14.5.2021
 * 
 *****************************************************************************/

#include "../test_lib/test_lib.h"
#include "../test_lib/TestBase.h"

using namespace test_lib;

#define TEST_DECLARATION(TEST_NAME) \
  class TEST_NAME : public test_lib::TestBase { \
    void ConfigureTest(); \
    int RunElemTest([[maybe_unused]] const ElementaryTest &elem_test, \
                    [[maybe_unused]] const TestVariant &test_variant); \
  };


TEST_DECLARATION(TestDemo)
