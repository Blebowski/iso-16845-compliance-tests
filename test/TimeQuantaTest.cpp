/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 29.5.2021
 * 
 * @brief Unit Test for "TimeQuanta" class
 *****************************************************************************/

#undef NDEBUG
#include <cassert>

#include "can_lib/TimeQuanta.h"

#include "can_lib/can.h"

using namespace can;


void test_forcing()
{
    TimeQuanta tq = TimeQuanta(nullptr, 10, BitPhase::Ph2);
    assert(tq.getLengthCycles() == 10);
    assert(tq.HasNonDefaultValues() == false);

    // Force all and check it was forced
    for (size_t i = 0; i < tq.getLengthCycles(); i++)
    {
        assert(tq.getCycleBitValue(i)->bit_value() == BitValue::Recessive);
        tq.ForceCycleValue(i, BitValue::Dominant);
        assert(tq.HasNonDefaultValues());
        assert(tq.getCycleBitValue(i)->bit_value() == BitValue::Dominant);
    }

    // Release all and check it has been released
    tq.SetAllDefaultValues();
    assert(tq.HasNonDefaultValues() == false);

    // Check forcing of all values at once
    TimeQuanta tq2 = TimeQuanta(nullptr, 10, BitPhase::Ph1);
    tq2.ForceValue(BitValue::Dominant);
    for (int i = 0; i < 10; i++)
        assert(tq2.getCycleBitValue(i)->has_default_value() == false);
}

void test_shorten_lengthen()
{
    TimeQuanta tq = TimeQuanta(nullptr, 10, BitPhase::Ph2, BitValue::Recessive);

    size_t sum = 0;
    for (int i = 0; i < 10; i++)
    {
        tq.Lengthen(i);
        sum += i;
        assert(tq.getLengthCycles() == sum + 10);
    }

    sum = tq.getLengthCycles();
    for (int i = 0; i < 10; i++)
    {
        tq.Shorten(i);
        sum -= i;
        assert(tq.getLengthCycles() == sum);
    }

    TimeQuanta tq2 = TimeQuanta(nullptr, 10, BitPhase::Ph2);
    tq2.Shorten(20);
    assert(tq2.getLengthCycles() == 0);
}

int main()
{
    test_forcing();
    test_shorten_lengthen();

    return 0;
}