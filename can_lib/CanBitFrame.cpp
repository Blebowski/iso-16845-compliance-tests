/*
 * TODO: License
 */

#include <iostream>
#include <assert.h>

#include "can.h"
#include "CanBit.h"
#include "CanBitFrame.h"

using namespace std;

CanBitFrame::CanBitFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                         RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                         ErrorStateIndicator isEsi, uint8_t dlc, int identifier,
                         uint8_t *data):
             CanFrame(isFdf, isIde, isRtr, isBrs, isEsi, dlc, identifier, data)
{
    buildFrameBits();
    cout << "CAN Frame before stuff bits: \n";
    print();

    if (isFdf_ == CAN_2_0){
        calculateCRC();
        insertNormalStuffBits();
    } else {
        insertNormalStuffBits();
        setStuffCount();
        setStuffParity();
        insertStuffCountStuffBits();
        calculateCRC();
        insertCrcFixedStuffBits();
    }
    print();
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
    for (int i = 4; i > 0; i--)
        push_bit(dlc_ >> i, BIT_TYPE_DLC);

    // Build data field
    for (int i = 0; i < getDataLenght(); i++) {
        for (int j = 7; j >= 0; j--)
            push_bit(getData(i) >> j, BIT_TYPE_DATA);
    }
    
    // Build Stuff count + parity (put dummy as we don't know number of
    // stuff bits yet)!
    for (int i = 0; i < 3; i++)
        bits_.push_back(CanBit(BIT_TYPE_STUFF_COUNT, DOMINANT));
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

int CanBitFrame::insertNormalStuffBits()
{
    std::list<CanBit>::iterator bit, firstFixedBit, lastNonFixedBit;
    int sameBits = 1;
    stuffCount_ = 0;
    BitValue prevValue = DOMINANT; // As if SOF

    if (bits_.front().bitType != BIT_TYPE_SOF) {
        std::cerr << "First bit of a frame should be SOF!" << std::endl;
        return 0;
    }

    if (bits_.size() < 5) {
        std::cerr << "At least 5 bits needed for bit stuffing!" << std::endl;
    }

    // Start from first bit of Base identifier
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
            sameBits++;
        else
            sameBits = 1;
        bit->stuffBitType = STUFF_NO;

        if (sameBits == 5)
        {
            BitValue stuffValue = bit->getOppositeValue();
            BitType stuffBitType = bit->bitType;
            bit++;
            bit = bits_.insert(bit, CanBit(stuffBitType, stuffValue));
            bit->stuffBitType = STUFF_NORMAL;
            sameBits = 1;

            stuffCount_ = (stuffCount_ + 1) % 8;
        }
        prevValue = bit->bitValue;
        lastNonFixedBit = bit;
    }

    return stuffCount_;
}


bool CanBitFrame::insertStuffCountStuffBits()
{
    std::list<CanBit>::iterator bit;
    BitValue stuffBitValue;

    if (getFdf() == CAN_2_0)
        return false;

    for (bit = bits_.begin(); bit->bitType != BIT_TYPE_STUFF_COUNT; bit++)
        ;
    bit--;
    stuffBitValue = bit->getOppositeValue();
    bit++;

    bit = bits_.insert(bit, CanBit(BIT_TYPE_STUFF_COUNT, stuffBitValue)); 
    bit->stuffBitType = STUFF_FIXED;

    // Move one beyond stuff parity and calculate stuff bit post parity
    for (int i = 0; i < 4; i++)
        bit++;
    stuffBitValue = bit->getOppositeValue();
    bit++;

    bit = bits_.insert(bit, CanBit(BIT_TYPE_STUFF_COUNT, stuffBitValue)); 
    bit->stuffBitType = STUFF_FIXED;

    return true;
}

void CanBitFrame::insertCrcFixedStuffBits()
{
    std::list<CanBit>::iterator bit;
    std::list<CanBit>::iterator lastNonFixedBit;
    int sameBits = 0;

    // Search first bit of CRC 
    for (bit = bits_.begin(); bit->bitType != BIT_TYPE_CRC; bit++)
        ;

    for (; bit->bitType != BIT_TYPE_CRC_DELIMITER; ++bit)
    {
        sameBits++;
        if ((sameBits % 4) == 0)
        {

            // Second fixed stuff bit is still in Stuff count field!
            // This is important for CRC calculation!
            BitType bitType;
            BitValue bitValue;
            
            bitType = BIT_TYPE_CRC;
            bitValue = bit->getOppositeValue();
            bit = bits_.insert(++bit, CanBit(bitType, bitValue));
            bit->stuffBitType = STUFF_FIXED;
        }
    }

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

bool CanBitFrame::setStuffCount()
{
    std::list<CanBit>::iterator bit;
    stuffCountEncoded_ = 0;
    bit = bits_.begin();

    // No sense to try to set Stuff count on CAN 2.0 frames!
    if (isFdf_ == CAN_2_0)
        return false;

    while (bit->bitType != BIT_TYPE_STUFF_COUNT && bit != bits_.end())
        bit++;

    if (bit == bits_.end())
    {
        std::cerr << "Did not find stuff count field!" << std::endl;
        return false;
    }

    assert(stuffCount_ < 8);

    switch (stuffCount_){
        case 0x0 :
            stuffCountEncoded_ = 0b000;
            break;
        case 0x1:
            stuffCountEncoded_ = 0b001;
            break;
        case 0x2 :
            stuffCountEncoded_ = 0b011;
            break;
        case 0x3 :
            stuffCountEncoded_ = 0b010;
            break;
        case 0x4 :
            stuffCountEncoded_ = 0b110;
            break;
        case 0x5 :
            stuffCountEncoded_ = 0b111;
            break;
        case 0x6 :
            stuffCountEncoded_ = 0b101;
            break;
        case 0x7 :
            stuffCountEncoded_ = 0b100;
            break;
    };

    for (int i = 2; i >= 0; i--)
    {
        assert(bit->bitType == BIT_TYPE_STUFF_COUNT);
        bit->bitValue = (BitValue)((stuffCountEncoded_ >> i) & 0x1);
        bit++;
    }
    return true;
}

bool CanBitFrame::setStuffParity()
{
    std::list<CanBit>::iterator bit;
    uint8_t val = 0;

    if (isFdf_ == CAN_2_0)
        return false;

    for (bit = bits_.begin(); bit->bitType != BIT_TYPE_STUFF_PARITY; bit++)
        ;
    for (int i = 0; i < 3; i++)
        val ^= (stuffCountEncoded_ >> i) & 0x1;
    bit->setBitValue((BitValue)val);

    // We must correct value of Fixed stuff bit after the stuff parity!

    return true;
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

int CanBitFrame::getBitIndex(CanBit *canBit)
{
    std::list<CanBit>::iterator bit = bits_.begin();
    int i = 0;

    while (&(*bit) != canBit && bit != bits_.end()) {
        i++;
        bit++;
    }
    return i;
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
    return insertActiveErrorFrame(getBitIndex(canBit));
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
    return insertPassiveErrorFrame(getBitIndex(canBit));
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
    return insertOverloadFrame(getBitIndex(canBit));
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
        std:
        cerr << "Can't loose arbitration on " << canBit->bitType << std::endl;
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
    return looseArbitration(getBitIndex(canBit));
}


void CanBitFrame::print()
{
    list<CanBit>::iterator bit;

    std::string vals = "";
    std::string names = "";

    for (bit = bits_.begin(); bit != bits_.end();)
    {
        // Print separators betwen different field types (also prints separator
        //  at start of frame)
        vals += "|";
        names += " ";

        // Both methods advance iterator when bit is printed.
        if (bit->isSingleBitField())
            printSingleBitField(bit, &vals, &names);
        else
            printMultiBitField(bit, &vals, &names);
    }

    std::cout << names << std::endl;
    std::cout << string(names.length(), '-') << std::endl;
    std::cout << vals << std::endl;
    std::cout << string(names.length(), '-') << std::endl;
}



void CanBitFrame::printSingleBitField(list<CanBit>::iterator& bit, string *vals,
                         string *names)
{
    list<CanBit>::iterator nxtBit;
    nxtBit = bit;
    nxtBit++;
    vals->append(" " + bit->getStringValue()+ " ");


    // Assumes name length is 3 otherwise lines will not be aligned...
    names->append(bit->getBitTypeName());
    bit++;

    // Handle stuff bit. If stuff bit is inserted behind a single bit
    // field it is marked with the same bit field!
    if (nxtBit->bitType == bit->bitType &&
        (nxtBit->stuffBitType == STUFF_FIXED ||
         nxtBit->stuffBitType == STUFF_NORMAL))
    {
        names->append(std::string(3, ' '));
        vals->append(" " + bit->getStringValue() + " ");
        bit++;
    }
}

void CanBitFrame::printMultiBitField(list<CanBit>::iterator& bit, string *vals,
                                     string *names)
{
    int len = 0;
    int preOffset = 0;
    int postOffset = 0;
    string fieldName = bit->getBitTypeName();
    list<CanBit>::iterator firstBit = bit;

    for (; bit->bitType == firstBit->bitType; bit++)
    {
        len += 2;
        vals->append(bit->getStringValue() + " ");
    }

    preOffset = (len - fieldName.length()) / 2;
    postOffset = preOffset;
    if (fieldName.length() % 2 == 1)
        postOffset++;

    // Do best effort here, if name is longer, keep no offset
    if (postOffset < 0)
        postOffset = 0;
    if (preOffset < 0)
        preOffset = 0;

    names->append(string(preOffset, ' '));
    names->append(fieldName);
    names->append(string(postOffset, ' '));
}