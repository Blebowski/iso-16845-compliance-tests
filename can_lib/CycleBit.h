/*
 * TODO: License
 */

#include <iostream>
#include <cstdint>
#include <list>

#include "can.h"
#include "Bit.h"
#include "CycleBitValue.h"

#ifndef CYCLE_BIT
#define CYCLE_BIT

class can::CycleBit : public Bit
{
    public:
        CycleBit();
        //CanBit(BitType bitType, BitValue bitValue);
        //CanBit(BitType bitType, BitValue bitValue, StuffBitType stuffBitType);

        std::list<CycleBitValue> cycleBitValues;
};

#endif