/*
 * TODO: License
 */

#include <iostream>

#include "can.h"
#include "Bit.h"

can::Bit::Bit()
{
    this->bitType = BitType::BIT_TYPE_IDLE;
    this->bitValue = BitValue::RECESSIVE;
}

can::Bit::Bit(BitType bitType, BitValue bitValue)
{
    this->bitType = bitType;
    this->bitValue = bitValue;
    this->stuffBitType = STUFF_NO;
}

can::Bit::Bit(BitType bitType, BitValue bitValue, StuffBitType stuffBitType)
{
    this->bitType = bitType;
    this->bitValue = bitValue;
    this->stuffBitType = stuffBitType;
}

can::BitValue can::Bit::getbitValue()
{
    return bitValue;
}

bool can::Bit::setBitValue(BitValue bitValue)
{
    this->bitValue = bitValue;
}

void can::Bit::flipBitValue()
{
    bitValue = getOppositeValue();
}

can::BitValue can::Bit::getOppositeValue()
{
    if (bitValue == DOMINANT)
        return RECESSIVE;
    return DOMINANT;
}

bool can::Bit::isStuffBit()
{
    if (stuffBitType == STUFF_NORMAL || stuffBitType == STUFF_FIXED)
        return true;
    return false;
}

std::string can::Bit::getBitTypeName()
{
    for (int i = 0; i < sizeof(bitTypeNames) / sizeof(BitTypeName); i++)
        if (bitTypeNames[i].bitType == bitType)
            return bitTypeNames[i].name;
    return " ";
}

std::string can::Bit::getStringValue()
{
    if (isStuffBit())
        return "\033[1;32m" + std::to_string((int)bitValue) + "\033[0m";
    else
        return std::to_string((int)bitValue);
}

bool can::Bit::isSingleBitField()
{
    if (bitType == BIT_TYPE_SOF ||
        bitType == BIT_TYPE_R0 ||
        bitType == BIT_TYPE_R1 ||
        bitType == BIT_TYPE_SRR ||
        bitType == BIT_TYPE_RTR ||
        bitType == BIT_TYPE_IDE ||
        bitType == BIT_TYPE_EDL ||
        bitType == BIT_TYPE_BRS ||
        bitType == BIT_TYPE_ESI ||
        bitType == BIT_TYPE_CRC_DELIMITER ||
        bitType == BIT_TYPE_STUFF_PARITY ||
        bitType == BIT_TYPE_ACK ||
        bitType == BIT_TYPE_ACK_DELIMITER)
        return true;

    return false;
}