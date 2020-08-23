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

#include "can.h"

#include "TimeQuanta.h"
#include "CycleBitValue.h"


can::TimeQuanta::TimeQuanta(int brp, BitPhase bit_phase)
{
    for (int i = 0; i < brp; i++)
        cycle_bit_values_.push_back(CycleBitValue());
    this->bit_phase = bit_phase;
}


can::TimeQuanta::TimeQuanta(int brp, BitPhase bit_phase, BitValue bit_value)
{
    for (int i = 0; i < brp; i++)
        cycle_bit_values_.push_back(CycleBitValue(bit_value));
    this->bit_phase = bit_phase;
}


bool can::TimeQuanta::HasNonDefaultValues()
{
    for (auto cycleBitValue : cycle_bit_values_)
        if (cycleBitValue.has_default_value_ = false)
            return true;

    return false;
}


void can::TimeQuanta::SetAllDefaultValues()
{
    for (auto cycleBitValue : cycle_bit_values_)
        cycleBitValue.ReleaseValue();
}


int can::TimeQuanta::getLengthCycles()
{
    return cycle_bit_values_.size();
}


can::CycleBitValue* can::TimeQuanta::getCycleBitValue(int index)
{
    auto iterator = cycle_bit_values_.begin();
    std::advance(iterator, index);
    return &(*iterator);
}

void can::TimeQuanta::Lengthen(int by_cycles)
{
    for (int i = 0; i < by_cycles; i++)
        cycle_bit_values_.push_back(CycleBitValue());
}


void can::TimeQuanta::Lengthen(int by_cycles, BitValue bit_value)
{
    for (int i = 0; i < by_cycles; i++)
        cycle_bit_values_.push_back(CycleBitValue(bit_value));
}


void can::TimeQuanta::Shorten(int by_cycles)
{
    if (by_cycles > cycle_bit_values_.size())
    {
        cycle_bit_values_.clear();
        return;
    }
    for (int i = 0; i < by_cycles; i++)
        cycle_bit_values_.pop_back();
}


bool can::TimeQuanta::ForceCycleValue(int cycle_index, BitValue bit_value)
{
    std::list<CycleBitValue>::iterator cycle_bit_value_iterator;
    cycle_bit_value_iterator = cycle_bit_values_.begin();

    if (cycle_index >= cycle_bit_values_.size())
        return false;

    std::advance(cycle_bit_value_iterator, cycle_index);
    cycle_bit_value_iterator->ForceValue(bit_value);

    return true;
}


bool can::TimeQuanta::ForceCycleValue(int cycle_index_from, int cycle_index_to,
                                      BitValue bit_value)
{
    int indexTo = cycle_index_to;
    std::list<CycleBitValue>::iterator cycle_bit_value_iterator;

    if (cycle_index_from >= cycle_bit_values_.size())
        return false;

    if (cycle_index_from + cycle_index_to >= cycle_bit_values_.size())
        indexTo = cycle_bit_values_.size() - 1;

    cycle_bit_value_iterator = cycle_bit_values_.begin();
    std::advance(cycle_bit_value_iterator, cycle_index_from);

    for (int i = 0; i < indexTo - cycle_index_from; i++, cycle_bit_value_iterator++)
        cycle_bit_value_iterator->ForceValue(bit_value);

    return true;
}


bool can::TimeQuanta::ForceValue(BitValue bit_value)
{
    std::list<CycleBitValue>::iterator cycle_bit_value_iterator;

    for (cycle_bit_value_iterator = cycle_bit_values_.begin();
         cycle_bit_value_iterator != cycle_bit_values_.end();
         cycle_bit_value_iterator++)
        cycle_bit_value_iterator->ForceValue(bit_value);
}