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

#include <assert.h>
#include "can.h"

#include "TimeQuanta.h"
#include "Cycle.h"


can::TimeQuanta::TimeQuanta(Bit *parent, int brp, BitPhase phase):
    phase_(phase),
    parent_(parent)
{
    for (int i = 0; i < brp; i++)
        cycles_.push_back(Cycle(this));
}


can::TimeQuanta::TimeQuanta(Bit *parent, int brp, BitPhase phase, BitVal val):
    phase_(phase),
    parent_(parent)
{
    for (int i = 0; i < brp; i++)
        cycles_.push_back(Cycle(this, val));
}


bool can::TimeQuanta::HasNonDefVals()
{
    for (auto & cycle : cycles_)
        if (!cycle.has_def_val())
            return true;
    return false;
}


void can::TimeQuanta::SetAllDefVals()
{
    for (auto & cycle_ : cycles_)
        cycle_.ReleaseVal();
}


size_t can::TimeQuanta::getLengthCycles()
{
    return cycles_.size();
}


std::list<can::Cycle>::iterator can::TimeQuanta::GetCycleBitValIter(size_t index)
{
    assert("Cycle index does not exist!" && index < cycles_.size());
    auto iterator = cycles_.begin();
    std::advance(iterator, index);
    return iterator;
}


can::Cycle* can::TimeQuanta::getCycleBitValue(size_t index)
{
    return &(*GetCycleBitValIter(index));
}


void can::TimeQuanta::Lengthen(size_t by_cycles)
{
    for (size_t i = 0; i < by_cycles; i++)
        cycles_.push_back(Cycle(this));
}


void can::TimeQuanta::Lengthen(size_t by_cycles, BitVal bit_value)
{
    for (size_t i = 0; i < by_cycles; i++)
        cycles_.push_back(Cycle(this, bit_value));
}


void can::TimeQuanta::Shorten(size_t by_cycles)
{
    size_t by_cycles_constrained = (cycles_.size() > by_cycles) ?
        by_cycles : cycles_.size();

    for (size_t i = 0; i < by_cycles_constrained; i++)
        cycles_.pop_back();
}


void can::TimeQuanta::ForceCycleValue(size_t cycle_index, BitVal bit_value)
{
    auto cycle_bit_value_iterator = GetCycleBitValIter(cycle_index);
    cycle_bit_value_iterator->ForceVal(bit_value);
}


void can::TimeQuanta::ForceVal(BitVal bit_value)
{
    for (auto & cycle_bit_value : cycles_)
        cycle_bit_value.ForceVal(bit_value);
}