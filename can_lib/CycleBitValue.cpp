/*
 * TODO: License
 */

#include <iostream>

#include "can.h"
#include "CycleBitValue.h"

can::CycleBitValue::CycleBitValue()
{
    hasDefaultValue = true;
    // bitValue is effectively don't care, initialize it just for correctness!
    bitValue = RECESSIVE;
}


can::CycleBitValue::CycleBitValue(BitValue bitValue)
{
    hasDefaultValue = false;
    this->bitValue = bitValue;
}


void can::CycleBitValue::forceValue(BitValue bitValue)
{
    hasDefaultValue = false;
    this->bitValue = bitValue;
}

void can::CycleBitValue::releaseValue()
{
    hasDefaultValue = true;
}