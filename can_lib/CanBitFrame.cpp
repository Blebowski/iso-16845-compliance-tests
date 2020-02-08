/*
 * TODO: License
 */

#include <iostream>
#include <assert.h>

#include "CanBitFrame.h"

CanBitFrame::CanBitFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                         RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                         ErrorStateIndicator isEsi, uint8_t dlc, int identifier,
                         uint8_t *data):
             CanFrame(isFdf, isIde, isRtr, isBrs, isEsi, dlc, identifier, data)
{
    buildFrameBits();
    insertStuffBits();
    insertStuffCount();
    calculateCRC();
}

uint32_t CanBitFrame::getBaseIdentifier()
{
    if (isIde_ == EXTENDED_IDENTIFIER)
        return ((uint32_t)(getIdentifier() >> 18));
    else
        return (uint32_t)getIdentifier();
}

uint32_t CanBitFrame::getIdentifierExtension()
{
    if (isIde_ == EXTENDED_IDENTIFIER)
        return (uint32_t)(getIdentifier()) & 0x3FFF;
    else
        return 0;
}

uint8_t CanBitFrame::getStuffCount()
{
    if (isFdf_ == CAN_2_0){
        std::cout << "CAN 2.0 frame does not have Stuff count field defined" << std::endl;
        return 0;
    }
    return stuffCount_;
}

uint32_t CanBitFrame::getCrc()
{
    if (isFdf_ == CAN_FD)
        return crc15_;
    if (dataLenght_ > 16)
        return crc21_;
    return crc17_;
}

// LSB represents bit value we want to push
void CanBitFrame::push_bit(uint8_t bit_val, BitType bitType)
{
    if ((bit_val % 2) == 0)
        bits_.push_back(CanBit(bitType, DOMINANT));
    else
        bits_.push_back(CanBit(bitType, RECESSIVE));
}

void CanBitFrame::clearFrameBits()
{
    bits_.clear();
}

bool CanBitFrame::clearFrameBits(int index)
{
    int i = 0;
    std::list<CanBit>::iterator bit;
    std::list<CanBit>::iterator end;

    if (index >= bits_.size())
        return false;

    bit = bits_.begin();
    end = bits_.end();
    std::advance(bit, index);
    bits_.erase(bit, end);
}

void CanBitFrame::buildFrameBits()
{
    clearFrameBits();
    bits_.push_back(CanBit(BIT_TYPE_SOF, DOMINANT));

    // Build base ID
    uint32_t baseId = getBaseIdentifier();
    for (int i = 11; i > 0; i--)
        push_bit(baseId >> i, BIT_TYPE_BASE_ID);

    // Build RTR/r1/SRR
    if (isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(CanBit(BIT_TYPE_SRR, RECESSIVE));
    } else {
        if (isFdf_ == CAN_FD) {
            bits_.push_back(CanBit(BIT_TYPE_R1, DOMINANT));
        } else {
            if (isRtr_ == RTR_FRAME)
                bits_.push_back(CanBit(BIT_TYPE_RTR, RECESSIVE));
            else
                bits_.push_back(CanBit(BIT_TYPE_RTR, DOMINANT));
        }
    }

    // Build IDE, Extended Identifier and one bit post Extended Identifier
    if (isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(CanBit(BIT_TYPE_IDE, DOMINANT));

        uint32_t extId = getIdentifierExtension();
        for (int i = 18; i > 0; i--)
            push_bit(extId >> i, BIT_TYPE_EXTENDED_ID);

        if (isFdf_ == CAN_FD) {
            bits_.push_back(CanBit(BIT_TYPE_R1, DOMINANT));
        } else {
            if (isRtr_ == RTR_FRAME) {
                bits_.push_back(CanBit(BIT_TYPE_RTR, RECESSIVE));
            } else {
                bits_.push_back(CanBit(BIT_TYPE_RTR, DOMINANT));
            }
        }
    } else {
        bits_.push_back(CanBit(BIT_TYPE_IDE, RECESSIVE));
    }

    // Build EDL/r0/r1 bit
    if (isFdf_ == CAN_FD) {
        bits_.push_back(CanBit(BIT_TYPE_EDL, RECESSIVE));
    } else if (isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(CanBit(BIT_TYPE_R1, DOMINANT));
    } else {
        bits_.push_back(CanBit(BIT_TYPE_R0, DOMINANT));
    }

    // Build extra r0 past EDL or in Extended Identifier frame
    if (isFdf_ == CAN_FD || isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(CanBit(BIT_TYPE_R0, DOMINANT));
    }

    // Build BRS and ESI bits
    if (isFdf_ == CAN_FD) {
        if (isBrs_ == BIT_RATE_SHIFT)
            bits_.push_back(CanBit(BIT_TYPE_BRS, RECESSIVE));
        else
            bits_.push_back(CanBit(BIT_TYPE_BRS, DOMINANT));
        
        if (isEsi_ == ESI_ERROR_ACTIVE)
            bits_.push_back(CanBit(BIT_TYPE_ESI, DOMINANT));
        else
            bits_.push_back(CanBit(BIT_TYPE_ESI, RECESSIVE));
    }

    // Build DLC
    for (int i = 4; i > 0; i++)
        push_bit(dlc_ >> i, BIT_TYPE_DLC);

    // Build data field
    for (int i = 0; i < getDataLenght(); i++) {
        for (int j = 7; j >= 0; j--)
            push_bit(getData(i) >> j, BIT_TYPE_DATA);
    }

    // Build Stuff count + parity (put dummy as we don't know number of
    // stuff bits yet)!
    for (int i = 0; i < 3; i++)
        bits_.push_back(CanBit(BIT_TYPE_STUFF_COUNT, RECESSIVE));
    bits_.push_back(CanBit(BIT_TYPE_STUFF_PARITY, RECESSIVE));

    // Calculate CRC (it is OK from what we have built so far...) and build it
    int crcLength;
    uint32_t crc = calculateCRC();

    if (getFdf() == CAN_2_0)
        crcLength = 15;
    else if (getDataLenght() <= 16)
        crcLength = 17;
    else
        crcLength = 21;
    for (int i = crcLength - 1; i >= 0; i--)
        push_bit(crc >> i, BIT_TYPE_CRC);

    // Add CRC Delimiter, ACK and ACK Delimiter
    bits_.push_back(CanBit(BIT_TYPE_CRC_DELIMITER, RECESSIVE));
    bits_.push_back(CanBit(BIT_TYPE_ACK, RECESSIVE));
    bits_.push_back(CanBit(BIT_TYPE_ACK_DELIMITER, RECESSIVE));

    // Finalize by EOF and by Intermission
    for (int i = 0; i < 7; i++)
        bits_.push_back(CanBit(BIT_TYPE_EOF, RECESSIVE));
    for (int i = 0; i < 3; i++)
        bits_.push_back(CanBit(BIT_TYPE_INTERMISSION, RECESSIVE));
}

int CanBitFrame::insertStuffBits()
{
    std::list<CanBit>::iterator bit, firstFixedBit, lastNonFixedBit;
    int same_bits = 1;
    stuffCount_ = 0;
    BitValue prevValue = DOMINANT; // As if SOF

    if (bits_.front().bitType != BIT_TYPE_SOF) {
        std::cerr << "First bit of a frame should be SOF!" << std::endl;
        return 0;
    }

    if (bits_.size() < 5) {
        std::cerr << "At least 5 bits needed for bit stuffing!" << std::endl;
    }

    // First do "normal" bit stuffing, start from first bit of Base identifier
    for (bit = ++(bits_.begin());; ++bit)
    {
        // Break when we reach Stuff count (CAN FD) or CRC delimiter (CAN 2.0).
        // Account also improperly created frame so break on the end!
        if (bit->bitType == BIT_TYPE_CRC_DELIMITER ||
            bit->bitType == BIT_TYPE_STUFF_COUNT ||
            bit == bits_.end())
        {
            // Save iterator of first fixed stuff bit
            firstFixedBit = bit;
            break;
        }

        if (bit->bitValue == prevValue)
            same_bits++;
        else
            same_bits = 1;
        bit->stuffBitType = STUFF_NO;

        if (same_bits == 5)
        {
            BitValue stuffValue = bit->getOppositeValue();
            BitType stuffBitType = bit->bitType;
            bits_.insert(++bit, CanBit(stuffBitType, stuffValue));
            same_bits = 1;
            bit->stuffBitType = STUFF_NORMAL;
            stuffCount_ = (stuffCount_ + 1) % 8;
        }
        prevValue = bit->bitValue;
        lastNonFixedBit = bit;
    }

    // For CAN FD frames (they always have stuff count) do fixed stuffing
    // We support only ISO CAN FD, so FD frame always contains Stuff count!
    if (getFdf() == CAN_FD) {

        // Insert one just in the start of stuff count
        if (lastNonFixedBit->stuffBitType == STUFF_NO) {
            bits_.insert(firstFixedBit, CanBit(BIT_TYPE_STUFF_COUNT,
                         lastNonFixedBit->getOppositeValue()));
            firstFixedBit->stuffBitType = STUFF_FIXED;
        }
        same_bits = 1;

        for (bit = ++firstFixedBit; bit->bitType != BIT_TYPE_CRC_DELIMITER; ++bit)
        {
            same_bits++;
            if ((same_bits % 4) == 0){

                // Second fixed stuff bit is still in Stuff count field!
                // This is important for CRC calculation!
                BitType bitType;
                BitValue bitValue;
                if (same_bits == 4)
                    bitType = BIT_TYPE_STUFF_COUNT;
                else
                    bitType = BIT_TYPE_CRC;

                bitValue = bit->getOppositeValue();
                bits_.insert(++bit, CanBit(bitType, bitValue));
            }
        }
    }

    return stuffCount_;
}

uint32_t CanBitFrame::calculateCRC()
{
    std::list<CanBit>::iterator bit;
    uint32_t crcNxt15 = 0;
    uint32_t crcNxt17 = 0;
    uint32_t crcNxt21 = 0;

    crc15_ = 0x0;
    crc17_ = (1 << 17);
    crc21_ = (1 << 21);

    // CRC calculation as in CAN FD spec!
    bit = bits_.begin();
    while (true)
    {
        if (bit->bitType == BIT_TYPE_CRC || bit->bitType == BIT_TYPE_STUFF_COUNT)
            break;

        crcNxt15 = (int)(bit->bitValue) ^ ((crc15_ >> 15) & 0x1);
        crcNxt17 = (int)(bit->bitValue) ^ ((crc17_ >> 17) & 0x1);
        crcNxt21 = (int)(bit->bitValue) ^ ((crc15_ >> 21) & 0x1);

        // Shift left
        crc15_ = (crc15_ << 1) | ((uint32_t)(bit->bitValue) & 0x1);
        crc17_ = (crc17_ << 1) | ((uint32_t)(bit->bitValue) & 0x1);
        crc21_ = (crc21_ << 1) | ((uint32_t)(bit->bitValue) & 0x1);

        // Calculate by polynomial
        if (crcNxt15 == 1)
            crc15_ ^= 0xC599;
        if (crcNxt17 == 1)
            crc17_ ^= 0x3685B;
        if (crcNxt21 == 1)
            crc21_ ^= 0x302899;

        bit++;
    }

    if (isFdf_ == CAN_2_0)
        return crc15_;
    if (dataLenght_ <= 16)
        return crc17_;
    return crc21_;
}

bool CanBitFrame::insertStuffCount()
{
    std::list<CanBit>::iterator bit;

    bit = bits_.begin();
    
    // No sense to try to set Stuff count on CAN 2.0 frames!
    if (isFdf_ == CAN_2_0)
        return false;

    while (bit->bitType != BIT_TYPE_STUFF_COUNT || bit != bits_.end())
        bit++;

    // First Stuff count bit should be a stuff bit, ignore it!
    assert(bit->stuffBitType == STUFF_FIXED);
    bit++;

    for (int i = 2; i >= 0; i--)
    {
        assert(bit->bitType == BIT_TYPE_STUFF_COUNT);
        bit->bitValue = (BitValue)((stuffCount_ >> i) & 0x1);
        bit++;
    }
}

CanBit* CanBitFrame::getBit(int index)
{
    std::list<CanBit>::iterator bit = bits_.begin();

    if (bits_.size() <= index)
        return nullptr;

    std::advance(bit, index);
    return &(*bit);
}

CanBit* CanBitFrame::getBit(int index, BitType bitType)
{
    std::list<CanBit>::iterator bit = bits_.begin();
    int i = 0;

    while (i < index || bit != bits_.end())
        if (bit->bitType == bitType){
            i++;
            bit++;
        }

    if (bit == bits_.end())
        return nullptr;

    return &(*bit);
}

CanBit* CanBitFrame::getStuffBit(int index)
{
    std::list<CanBit>::iterator bit = bits_.begin();
    int i = 0;

    while (i < index || bit != bits_.end())
        if (bit->stuffBitType == STUFF_NORMAL){
            i++;
            bit++;
        }

    if (bit == bits_.end())
        return nullptr;

    return &(*bit);
}

CanBit* CanBitFrame::getFixedStuffBit(int index)
{
    std::list<CanBit>::iterator bit = bits_.begin();
    int i = 0;

    while (i < index || bit != bits_.end())
        if (bit->stuffBitType == STUFF_FIXED){
            i++;
            bit++;
        }

    if (bit == bits_.end())
        return nullptr;

    return &(*bit);
}

bool CanBitFrame::insertBit(CanBit canBit, int index)
{
    if (index >= bits_.size())
        return false;

    std::list<CanBit>::iterator bit;
    std::advance(bit, index);
    bits_.insert(bit, canBit);

    return true;
}

bool CanBitFrame::removeBit(CanBit *canBit)
{
    std::list<CanBit>::iterator bit = bits_.begin();
    while (bit != bits_.end())
    {
        if (&(*bit) == canBit)
            break;
        bit++;
    };
    bits_.erase(bit);
}

bool CanBitFrame::removeBit(int index)
{
    std::list<CanBit>::iterator bit;

    if (bits_.size() <= index)
        return false;

    std::advance(bit, index);
    bits_.erase(bit);

    return true;
}

bool CanBitFrame::insertAck()
{
    // This assumes only first ACK bit is set. In case we have more of them
    // (like prolonged ACK in CAN FD frame, we do only the first one)    
    CanBit *canBit = getBit(0, BIT_TYPE_ACK);

    if (canBit == nullptr)
        return false;
    
    canBit->setBitValue(DOMINANT);
    return true;
}

bool CanBitFrame::insertActiveErrorFrame(int index)
{
    CanBit *canBit = getBit(index);

    if (canBit == nullptr)
        return false;

    // Discard all bits from this bit further
    clearFrameBits(index);

    // Insert Active Error flag and Error delimiter
    for (int i = 0; i < 6; i++)
        bits_.push_back(CanBit(BIT_TYPE_ACTIVE_ERROR_FLAG, DOMINANT));
    for (int i = 0; i < 8; i++)
        bits_.push_back(CanBit(BIT_TYPE_ERROR_DELIMITER, RECESSIVE));

    return true;
}

bool CanBitFrame::insertActiveErrorFrame(CanBit *canBit)
{
    return insertActiveErrorFrame(getBitIndex(*canBit));
}

bool CanBitFrame::insertPassiveErrorFrame(int index)
{
    CanBit *canBit = getBit(index);

    if (canBit == nullptr)
        return false;

    // Discard all bits from this bit further
    clearFrameBits(index);

    for (int i = 0; i < 6; i++)
        bits_.push_back(CanBit(BIT_TYPE_PASSIVE_ERROR_FLAG, RECESSIVE));
    for (int i = 0; i < 8; i++)
        bits_.push_back(CanBit(BIT_TYPE_ERROR_DELIMITER, RECESSIVE));

    return true;
}

bool CanBitFrame::insertPassiveErrorFrame(CanBit *canBit)
{
    return insertPassiveErrorFrame(getBitIndex(*canBit));
}

bool CanBitFrame::insertOverloadFrame(int index)
{
    CanBit *canBit = getBit(index);

    if (canBit == nullptr)
        return false;

    if (canBit->bitType != BIT_TYPE_INTERMISSION &&
        canBit->bitType != BIT_TYPE_ERROR_DELIMITER &&
        canBit->bitType != BIT_TYPE_OVERLOAD_DELIMITER)
    {
        std::cerr << " Can't insert Overload frame on " << canBit->bitType <<
            std::endl;
        return false;
    }

    for (int i = 0; i < 6; i++)
        bits_.push_back(CanBit(BIT_TYPE_OVERLOAD_FLAG, DOMINANT));
    for (int i = 0; i < 8; i++)
        bits_.push_back(CanBit(BIT_TYPE_OVERLOAD_DELIMITER, RECESSIVE));
    return true;
}

bool CanBitFrame::insertOverloadFrame(CanBit *canBit)
{
    return insertOverloadFrame(getBitIndex(*canBit));
}

bool CanBitFrame::looseArbitration(int index)
{
    CanBit *canBit = getBit(index);
    list<CanBit>::iterator bit = bits_.begin();

    if (canBit == nullptr)
        return false;

    if (canBit->bitType != BIT_TYPE_BASE_ID &&
        canBit->bitType != BIT_TYPE_EXTENDED_ID &&
        canBit->bitType != BIT_TYPE_RTR &&
        canBit->bitType != BIT_TYPE_SRR &&
        canBit->bitType != BIT_TYPE_IDE)
    {
        std:cerr << "Can't loos arbitration on " << canBit->bitType << std::endl;
            return false;
    }

    // Move to position where we want to loose arbitration
    for (int i = 0; i < index; i++)
        bit++;

    for (; bit != bits_.end(); bit++){
        if (bit->bitType == BIT_TYPE_ACK)
            bit->bitValue = DOMINANT;
        else
            bit->bitValue = RECESSIVE;
    }
    return true;
}

bool CanBitFrame::looseArbitration(CanBit *canBit)
{
    return looseArbitration(getBitIndex(*canBit));
}

void CanBitFrame::print()
{
    
}

