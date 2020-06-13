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

#include <iostream>

#include "can.h"
#include "BitTiming.h"
#include "TimeQuanta.h"
#include "Bit.h"


can::Bit::Bit(BitType bitType, BitValue bitValue, FrameFlags* frameFlags,
              BitTiming* nominalBitTiming, BitTiming* dataBitTiming)
{
    this->bitType = bitType;
    this->bitValue = bitValue;
    this->stuffBitType = STUFF_NO;

    this->frameFlags = frameFlags;
    this->nominalBitTiming = nominalBitTiming;
    this->dataBitTiming = dataBitTiming;

    constructTimeQuantas(nominalBitTiming, dataBitTiming);
}


can::Bit::Bit(BitType bitType, BitValue bitValue, FrameFlags* frameFlags,
              BitTiming* nominalBitTiming, BitTiming* dataBitTiming,
              StuffBitType stuffBitType)
{
    this->bitType = bitType;
    this->bitValue = bitValue;
    this->stuffBitType = stuffBitType;
    
    this->frameFlags = frameFlags;
    this->nominalBitTiming = nominalBitTiming;
    this->dataBitTiming = dataBitTiming;

    constructTimeQuantas(nominalBitTiming, dataBitTiming);
}


can::BitValue can::Bit::getBitValue()
{
    return bitValue;
}


can::BitType can::Bit::getBitType()
{
    return bitType;
}


can::StuffBitType can::Bit::getStuffBitType()
{
    return stuffBitType;
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
    else if (bitType == BIT_TYPE_ACTIVE_ERROR_FLAG ||
             bitType == BIT_TYPE_PASSIVE_ERROR_FLAG ||
             bitType == BIT_TYPE_ERROR_DELIMITER)
        return "\033[1;31m" + std::to_string((int)bitValue) + "\033[0m";
    else if (bitType == BIT_TYPE_OVERLOAD_FLAG ||
             bitType == BIT_TYPE_OVERLOAD_DELIMITER)
        return "\033[1;36m" + std::to_string((int)bitValue) + "\033[0m";
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


bool can::Bit::hasPhase(BitPhase bitPhase)
{
    for (auto timeQuanta : timeQuantas_)
        if (timeQuanta.bitPhase == bitPhase)
            return true;

    return false;
}


bool can::Bit::hasNonDefaultValues()
{
    for (auto timeQuanta : timeQuantas_)
        if (timeQuanta.hasNonDefaultValues())
            return true;

    return false;
}


void can::Bit::setAllDefaultValues()
{
    for (auto timeQuanta : timeQuantas_)
        timeQuanta.setAllDefaultValues();
}


int can::Bit::getPhaseLenTimeQuanta(BitPhase bitPhase)
{
    int numTimeQuanta = 0;

    for (auto timeQuanta : timeQuantas_)
        if (timeQuanta.bitPhase == bitPhase)
            numTimeQuanta++;

    return numTimeQuanta;
}


int can::Bit::getPhaseLenCycles(BitPhase bitPhase)
{
    int numCycles = 0;

    for (auto timeQuanta : timeQuantas_)
        if (timeQuanta.bitPhase == bitPhase)
            numCycles += timeQuanta.getLengthCycles();

    return numCycles;
}


int can::Bit::getLenTimeQuanta()
{
    int numTimeQuanta = 0;

    numTimeQuanta += getPhaseLenTimeQuanta(SYNC_PHASE);
    numTimeQuanta += getPhaseLenTimeQuanta(PROP_PHASE);
    numTimeQuanta += getPhaseLenTimeQuanta(PH1_PHASE);
    numTimeQuanta += getPhaseLenTimeQuanta(PH2_PHASE);

    return numTimeQuanta;
}


int can::Bit::getLenCycles()
{
    int numCycles = 0;

    numCycles += getPhaseLenCycles(SYNC_PHASE);
    numCycles += getPhaseLenCycles(PROP_PHASE);
    numCycles += getPhaseLenCycles(PH1_PHASE);
    numCycles += getPhaseLenCycles(PH2_PHASE);

    return numCycles;
}


int can::Bit::shortenPhase(BitPhase bitPhase, int numTimeQuanta)
{
    int phaseLen = getPhaseLenTimeQuanta(bitPhase);
    int shortenBy = numTimeQuanta;

    printf("Phase lenght: %d\n", phaseLen);
    printf("Shorten by: %d\n", shortenBy);

    if (phaseLen == 0)
        return 0;
    if (phaseLen < numTimeQuanta)
        shortenBy = phaseLen;

    auto timeQuantaIterator = getLastTimeQuantaIterator(bitPhase);
    for (int i = 0; i < shortenBy; i++){
        timeQuantaIterator = timeQuantas_.erase(timeQuantaIterator);
        timeQuantaIterator--;
    }

    return shortenBy;
}


void can::Bit::lengthenPhase(BitPhase bitPhase, int numTimeQuanta)
{
    int phaseLen = getPhaseLenTimeQuanta(bitPhase);
    auto timeQuantaIterator = getLastTimeQuantaIterator(bitPhase);

    // GetLastTimeQuantaIterator returns Last time Quanta of phase or
    // first of next phase if phase does not exist. If it exist, move
    // to one further so that we append to the end
    if (phaseLen > 0)
        timeQuantaIterator++;

    BitTiming *bitTiming = getPhaseBitTiming(bitPhase);

    for (int i = 0; i < numTimeQuanta; i++)
        timeQuantas_.insert(timeQuantaIterator,
                            TimeQuanta(bitTiming->brp, bitPhase));
}


can::TimeQuanta* can::Bit::getTimeQuanta(int index)
{
    assert(index < timeQuantas_.size());

    auto timeQuantaIterator = timeQuantas_.begin();
    std::advance(timeQuantaIterator, index);

    return &(*timeQuantaIterator);
}


can::TimeQuanta* can::Bit::getTimeQuanta(BitPhase bitPhase, int index)
{
    int phaseLen = getPhaseLenTimeQuanta(bitPhase);
    int realIndex = index;
    assert(phaseLen > 0);

    auto timeQuantaIterator = getFirstTimeQuantaIterator(bitPhase);

    // Saturate
    if (index >= phaseLen)
        realIndex = phaseLen - 1;
    std::advance(timeQuantaIterator, realIndex);

    return &(*timeQuantaIterator);
}


bool can::Bit::forceTimeQuanta(int index, BitValue bitValue)
{
    if (index >= getLenTimeQuanta())
        return false;

    auto timeQuantaIterator = timeQuantas_.begin();
    std::advance(timeQuantaIterator, index);
    timeQuantaIterator->forceValue(bitValue);

    return true;
}


int can::Bit::forceTimeQuanta(int fromIndex, int toIndex, BitValue bitValue)
{
    int lenTimeQuanta = getLenTimeQuanta();

    if (fromIndex >= lenTimeQuanta)
        return 0;
    if (fromIndex > toIndex)
        return 0;

    int toIndexReal = toIndex;
    if (toIndex >= lenTimeQuanta)
        toIndexReal = lenTimeQuanta - 1;

    auto timeQuantaIterator = timeQuantas_.begin();
    std::advance(timeQuantaIterator, fromIndex);

    int i = 0;
    for (; i <= toIndexReal - fromIndex; i++)
    {
        timeQuantaIterator->forceValue(bitValue);
        timeQuantaIterator++;
    }

    return i;
}


bool can::Bit::forceTimeQuanta(int index, BitPhase bitPhase, BitValue bitValue)
{
    int phaseLen = getPhaseLenTimeQuanta(bitPhase);

    if ((phaseLen == 0) || (phaseLen <= index))
        return false;

    TimeQuanta *timeQuanta = getTimeQuanta(bitPhase, index);
    timeQuanta->forceValue(bitValue);

    return true;
}


bool can::Bit::forceTimeQuanta(int fromIndex, int toIndex, BitPhase bitPhase,
                               BitValue bitValue)
{
    int phaseLen = getPhaseLenTimeQuanta(bitPhase);
    if (phaseLen = 0 || phaseLen <= fromIndex)
        return false;

    int toIndexReal = toIndex;
    if (phaseLen >= toIndex)
        toIndexReal = phaseLen - 1;

    auto timeQuantaIterator = getFirstTimeQuantaIterator(bitPhase);
    std::advance(timeQuantaIterator, fromIndex);

    int i = 0;
    for (; i < toIndexReal - fromIndex + 1; i++)
    {
        timeQuantaIterator->forceValue(bitValue);
        timeQuantaIterator++;
    }

    return i;
}


can::BitPhase can::Bit::prevBitPhase(BitPhase bitPhase)
{
    switch (bitPhase)
    {
    case PH2_PHASE:
        if (hasPhase(PH1_PHASE))
            return PH1_PHASE;
        if (hasPhase(PROP_PHASE))
            return PROP_PHASE;
        
        // Assume here Sync phase is always there. We can't remove
        // it by Bit time settings. Having bit without sync phase
        // means corrupted bit!
        assert(hasPhase(SYNC_PHASE));
        return SYNC_PHASE;

    case PH1_PHASE:
        if (hasPhase(PROP_PHASE))
            return PROP_PHASE;

        assert(hasPhase(SYNC_PHASE));
        return SYNC_PHASE;

    case PROP_PHASE:
        assert(hasPhase(SYNC_PHASE));
        return SYNC_PHASE;

    // In case of Sync phase do not link to previous bit in any way...
    case SYNC_PHASE:
        return SYNC_PHASE;
    }
}


can::BitPhase can::Bit::nextBitPhase(BitPhase bitPhase)
{
    switch (bitPhase)
    {
    case SYNC_PHASE:
        if (hasPhase(PROP_PHASE))
            return PROP_PHASE;
        if (hasPhase(PH1_PHASE))
            return PH1_PHASE;
        if (hasPhase(PH2_PHASE))
            return PH2_PHASE;
        return SYNC_PHASE;

    case PROP_PHASE:
        if (hasPhase(PH1_PHASE))
            return PH1_PHASE;
        if (hasPhase(PH2_PHASE))
            return PH2_PHASE;
        return PROP_PHASE;

    case PH1_PHASE:
        if (hasPhase(PH2_PHASE))
            return PH2_PHASE;
        return PH1_PHASE;
    case PH2_PHASE:
        return PH2_PHASE;
    }
}


can::BitRate can::Bit::getPhaseBitRate(BitPhase bitPhase)
{
    if (frameFlags->isFdf_ == CAN_FD && frameFlags->isBrs_ == BIT_RATE_SHIFT)
    {
        switch (bitType) {
        case BIT_TYPE_BRS :
            if (bitPhase == PH2_PHASE)
                return DATA_BIT_RATE;
            else
                return NOMINAL_BIT_RATE;

        case BIT_TYPE_CRC_DELIMITER :
            if (bitPhase == PH2_PHASE)
                return NOMINAL_BIT_RATE;
            else
                return DATA_BIT_RATE;

        case BIT_TYPE_ESI:
        case BIT_TYPE_DLC:
        case BIT_TYPE_DATA:
        case BIT_TYPE_STUFF_COUNT:
        case BIT_TYPE_STUFF_PARITY:
        case BIT_TYPE_CRC:
            return DATA_BIT_RATE;

        default:
            break;
        }
    }
    return NOMINAL_BIT_RATE;
}


can::BitTiming* can::Bit::getPhaseBitTiming(BitPhase bitPhase)
{
    BitRate bitRate = getPhaseBitRate(bitPhase);

    if (bitRate == NOMINAL_BIT_RATE)
        return nominalBitTiming;
    else
        return dataBitTiming;
}


void can::Bit::correctPh2LenToNominal()
{
    // If bit Phase 2 is in data bit rate, then correct it to nominal
    if (getPhaseBitTiming(PH2_PHASE) == dataBitTiming)
    {
        for (auto tqIter = timeQuantas_.begin(); tqIter != timeQuantas_.end();)
            if (tqIter->bitPhase == PH2_PHASE)
                tqIter = timeQuantas_.erase(tqIter);
            else
                tqIter++;

        for (int i = 0; i < nominalBitTiming->ph2; i++)
            timeQuantas_.push_back(TimeQuanta(nominalBitTiming->brp, PH2_PHASE));
    }
}


std::list<can::TimeQuanta>::iterator
can::Bit::getFirstTimeQuantaIterator(BitPhase bitPhase)
{
    if (hasPhase(bitPhase))
    {
        auto iterator = timeQuantas_.begin();
        while (iterator->bitPhase != bitPhase)
            iterator++;
        return iterator;
    }
    return getLastTimeQuantaIterator(prevBitPhase(bitPhase));
}


std::list<can::TimeQuanta>::iterator
    can::Bit::getLastTimeQuantaIterator(BitPhase bitPhase)
{
    if (hasPhase(bitPhase))
    {
        auto iterator = getFirstTimeQuantaIterator(bitPhase);
        while (iterator->bitPhase == bitPhase)
            iterator++;
        return --iterator;
    }
    return getFirstTimeQuantaIterator(nextBitPhase(bitPhase));
}


void can::Bit::constructTimeQuantas(BitTiming* nominalBitTiming,
                                    BitTiming* dataBitTiming)
{
    BitTiming *tseg1BitTiming = nominalBitTiming;
    BitTiming *tseg2BitTiming = nominalBitTiming;

    // Here Assume that PH1 has the same bit rate as TSEG1 which is reasonable
    // as there is no bit-rate shift within TSEG1
    BitRate tseg1BitRate = getPhaseBitRate(PH1_PHASE);
    BitRate tseg2BitRate = getPhaseBitRate(PH2_PHASE);

    if (tseg1BitRate == DATA_BIT_RATE)
        tseg1BitTiming = dataBitTiming;
    if (tseg2BitRate == DATA_BIT_RATE)
        tseg2BitTiming = dataBitTiming;

    // Construct TSEG 1
    timeQuantas_.push_back(TimeQuanta(tseg1BitTiming->brp, SYNC_PHASE));
    for (int i = 0; i < tseg1BitTiming->prop; i++)
        timeQuantas_.push_back(TimeQuanta(tseg1BitTiming->brp, PROP_PHASE));
    for (int i = 0; i < tseg1BitTiming->ph1; i++)
        timeQuantas_.push_back(TimeQuanta(tseg1BitTiming->brp, PH1_PHASE));

    // Construct TSEG 2
    for (int i = 0; i < tseg2BitTiming->ph2; i++)
        timeQuantas_.push_back(TimeQuanta(tseg2BitTiming->brp, PH2_PHASE));
}