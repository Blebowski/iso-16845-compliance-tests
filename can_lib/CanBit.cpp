/*
 * TODO: License
 */

#include <iostream>

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

bool CanBit::isStuffBit()
{
    if (stuffBitType == STUFF_NORMAL || stuffBitType == STUFF_FIXED)
        return true;
    return false;
}

std::string CanBit::getBitTypeName()
{
    for (int i = 0; i < sizeof(bitTypeNames) / sizeof(BitTypeName); i++)
        if (bitTypeNames[i].bitType == bitType)
            return bitTypeNames[i].name;
    return " ";
}

std::string CanBit::getStringValue()
{
    if (isStuffBit())
        return "\033[1;32m" + std::to_string((int)bitValue) + "\033[0m";
    else
        return std::to_string((int)bitValue);
}

bool CanBit::isSingleBitField()
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