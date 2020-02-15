/*
 * TODO: License
 */

#include "can.h"

#include "TimeQuanta.h"
#include "CycleBitValue.h"


can::TimeQuanta::TimeQuanta(int brp, BitPhase bitPhase)
{
    for (int i = 0; i < brp; i++)
        cycleBitValues_.push_back(CycleBitValue());
    this->bitPhase = bitPhase;
}


can::TimeQuanta::TimeQuanta(int brp, BitPhase bitPhase, BitValue bitValue)
{
    for (int i = 0; i < brp; i++)
        cycleBitValues_.push_back(CycleBitValue(bitValue));
    this->bitPhase = bitPhase;
}


bool can::TimeQuanta::hasNonDefaultValues()
{
    for (auto cycleBitValue : cycleBitValues_)
        if (cycleBitValue.hasDefaultValue = false)
            return true;

    return false;
}


void can::TimeQuanta::setAllDefaultValues()
{
    for (auto cycleBitValue : cycleBitValues_)
        cycleBitValue.releaseValue();
}


int can::TimeQuanta::getLengthCycles()
{
    return cycleBitValues_.size();
}


void can::TimeQuanta::lengthen(int byCycles)
{
    for (int i = 0; i < byCycles; i++)
        cycleBitValues_.push_back(CycleBitValue());
}


void can::TimeQuanta::lengthen(int byCycles, BitValue bitValue)
{
    for (int i = 0; i < byCycles; i++)
        cycleBitValues_.push_back(CycleBitValue(bitValue));
}


void can::TimeQuanta::shorten(int byCycles)
{
    if (byCycles > cycleBitValues_.size())
    {
        cycleBitValues_.clear();
        return;
    }
    for (int i = 0; i < byCycles; i++)
        cycleBitValues_.pop_back();
}


bool can::TimeQuanta::forceCycleValue(int cycleIndex, BitValue bitValue)
{
    std::list<CycleBitValue>::iterator cycleBitValueIterator;
    cycleBitValueIterator = cycleBitValues_.begin();

    if (cycleIndex >= cycleBitValues_.size())
        return false;

    std::advance(cycleBitValueIterator, cycleIndex);
    cycleBitValueIterator->forceValue(bitValue);

    return true;
}


bool can::TimeQuanta::forceCycleValue(int cycleIndexFrom, int cycleIndexTo,
                                      BitValue bitValue)
{
    int indexTo = cycleIndexTo;
    std::list<CycleBitValue>::iterator cycleBitValueIterator;

    if (cycleIndexFrom >= cycleBitValues_.size())
        return false;

    if (cycleIndexFrom + cycleIndexTo >= cycleBitValues_.size())
        indexTo = cycleBitValues_.size() - 1;

    cycleBitValueIterator = cycleBitValues_.begin();
    std::advance(cycleBitValueIterator, cycleIndexFrom);

    for (int i = 0; i < indexTo - cycleIndexFrom; i++, cycleBitValueIterator++)
        cycleBitValueIterator->forceValue(bitValue);

    return true;
}


bool can::TimeQuanta::forceValue(BitValue bitValue)
{
    std::list<CycleBitValue>::iterator cycleBitValueIterator;

    for (cycleBitValueIterator = cycleBitValues_.begin();
         cycleBitValueIterator != cycleBitValues_.end();
         cycleBitValueIterator++)
        cycleBitValueIterator->forceValue(bitValue);
}