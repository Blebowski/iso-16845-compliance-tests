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

#include <assert.h>
#include "can.h"

#include "TimeQuanta.h"
#include "CycleBitValue.h"


can::TimeQuanta::TimeQuanta(Bit *parent, int brp, BitPhase bit_phase):
    bit_phase_(bit_phase),
    parent_(parent)
{
    for (int i = 0; i < brp; i++)
        cycle_bit_values_.push_back(CycleBitValue(this));
}


can::TimeQuanta::TimeQuanta(Bit *parent, int brp, BitPhase bit_phase, BitValue bit_value):
    bit_phase_(bit_phase),
    parent_(parent)
{
    for (int i = 0; i < brp; i++)
        cycle_bit_values_.push_back(CycleBitValue(this, bit_value));
}


bool can::TimeQuanta::HasNonDefaultValues()
{
    for (auto & cycleBitValue : cycle_bit_values_)
        if (!cycleBitValue.has_default_value())
            return true;
    return false;
}


void can::TimeQuanta::SetAllDefaultValues()
{
    for (auto & cycleBitValue : cycle_bit_values_)
        cycleBitValue.ReleaseValue();
}


size_t can::TimeQuanta::getLengthCycles()
{
    return cycle_bit_values_.size();
}


std::list<can::CycleBitValue>::iterator can::TimeQuanta::GetCycleBitValueIterator(size_t index)
{
    assert("Cycle index does not exist!" && index < cycle_bit_values_.size());
    auto iterator = cycle_bit_values_.begin();
    std::advance(iterator, index);
    return iterator;
}


can::CycleBitValue* can::TimeQuanta::getCycleBitValue(size_t index)
{
    return &(*GetCycleBitValueIterator(index));
}


void can::TimeQuanta::Lengthen(size_t by_cycles)
{
    for (size_t i = 0; i < by_cycles; i++)
        cycle_bit_values_.push_back(CycleBitValue(this));
}


void can::TimeQuanta::Lengthen(size_t by_cycles, BitValue bit_value)
{
    for (size_t i = 0; i < by_cycles; i++)
        cycle_bit_values_.push_back(CycleBitValue(this, bit_value));
}


void can::TimeQuanta::Shorten(size_t by_cycles)
{
    size_t by_cycles_constrained = (cycle_bit_values_.size() > by_cycles) ?
        by_cycles : cycle_bit_values_.size();

    for (size_t i = 0; i < by_cycles_constrained; i++)
        cycle_bit_values_.pop_back();
}


void can::TimeQuanta::ForceCycleValue(size_t cycle_index, BitValue bit_value)
{
    auto cycle_bit_value_iterator = GetCycleBitValueIterator(cycle_index);
    cycle_bit_value_iterator->ForceValue(bit_value);
}


void can::TimeQuanta::ForceValue(BitValue bit_value)
{
    for (auto & cycle_bit_value : cycle_bit_values_)
        cycle_bit_value.ForceValue(bit_value);
}