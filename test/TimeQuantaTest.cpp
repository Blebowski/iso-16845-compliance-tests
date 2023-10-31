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
    assert(tq.HasNonDefVals() == false);

    // Force all and check it was forced
    for (size_t i = 0; i < tq.getLengthCycles(); i++)
    {
        assert(tq.getCycleBitValue(i)->bit_value() == BitVal::Recessive);
        tq.ForceCycleValue(i, BitVal::Dominant);
        assert(tq.HasNonDefVals());
        assert(tq.getCycleBitValue(i)->bit_value() == BitVal::Dominant);
    }

    // Release all and check it has been released
    tq.SetAllDefVals();
    assert(tq.HasNonDefVals() == false);

    // Check forcing of all values at once
    TimeQuanta tq2 = TimeQuanta(nullptr, 10, BitPhase::Ph1);
    tq2.ForceVal(BitVal::Dominant);
    for (int i = 0; i < 10; i++)
        assert(tq2.getCycleBitValue(i)->has_def_val() == false);
}

void test_shorten_lengthen()
{
    TimeQuanta tq = TimeQuanta(nullptr, 10, BitPhase::Ph2, BitVal::Recessive);

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