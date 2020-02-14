/*
 * TODO: License
 */

#include <iostream>

#include "can.h"
#include "CycleBitValue.h"

can::CycleBitValue::CycleBitValue()
{
    hasDefaultValue = true;
    bitValue = RECESSIVE;
}

can::CycleBitValue::CycleBitValue(BitValue bitValue)
{
    hasDefaultValue = false;
    bitValue = bitValue;
}