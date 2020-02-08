/*
 * TODO: License
 */

#include "can.h"
#include "CanBit.h"

using namespace can;

CanBit::CanBit()
{
    this->bitType = BitType::BIT_TYPE_IDLE;
    this->bitValue = BitValue::RECESSIVE;
}

CanBit::CanBit(BitType bitType, BitValue bitValue)
{
    this->bitType = bitType;
    this->bitValue = bitValue;
}

BitValue CanBit::getbitValue()
{
    return bitValue;
}

bool CanBit::setBitValue(BitValue bitValue)
{
    this->bitValue = bitValue;
}

void CanBit::flipBitValue()
{
    bitValue = getOppositeValue();
}

BitValue CanBit::getOppositeValue()
{
    if (bitValue == DOMINANT)
        return RECESSIVE;
    return DOMINANT;
}

bool CanBit::isStuffBit(CanBit canBit)
{
    if (stuffBitType == STUFF_NORMAL || stuffBitType == STUFF_FIXED)
        return true;
    return false;
}