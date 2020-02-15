/*
 * TODO: License
 */

#include <iostream>

#include "can.h"

#ifndef CYCLE_BIT_VALUE
#define CYCLE_BIT_VALUE


class can::CycleBitValue
{
    public:
        CycleBitValue();
        CycleBitValue(BitValue bitValue);

        void forceValue(BitValue bitValue);
        void releaseValue();

        // Default value from CanBit should be taken
        bool hasDefaultValue;

        // If not default value, then this value is taken instead
        BitValue bitValue;
};

#endif