/*
 * TODO: License
 */

#include "can.h"

using namespace can;

#ifndef CAN_BIT
#define CAN_BIT

class CanBit {

    public:
        CanBit();
        CanBit(BitType bitType, BitValue bitValue);

        BitType bitType;
        StuffBitType stuffBitType;
        BitValue bitValue;

        bool setBitValue(BitValue bitValue);
        BitValue getbitValue();
        void flipBitValue();
        bool isStuffBit(CanBit canBit);

        BitValue getOppositeValue();
};

#endif