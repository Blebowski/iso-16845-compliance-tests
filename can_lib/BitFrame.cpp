/*
 * TODO: License
 */

#include <iostream>
#include <assert.h>

#include "can.h"
#include "Bit.h"
#include "BitFrame.h"

can::BitFrame::BitFrame(FrameFlags frameFlags, uint8_t dlc, int identifier,
                        uint8_t *data):
                Frame(frameFlags, dlc, identifier, data)
{
    buildFrameBits();
    print(true);

    if (frameFlags.isFdf_ == CAN_2_0){
        calculateCrc();
        setCrc();

        // We must set CRC before Stuff bits are inserted because in CAN 2.0
        // frames regular stuff bits are inserted also to CRC!
        insertNormalStuffBits();
    } else {
        insertNormalStuffBits();
        setStuffCount();
        setStuffParity();
        insertStuffCountStuffBits();
        calculateCrc();
        setCrc();
        insertCrcFixedStuffBits();
    }
    print(true);
}

uint32_t can::BitFrame::setCrc()
{
    std::list<Bit>::iterator bitIt = getBitOfIterator(0, BIT_TYPE_CRC);
    int i = 0;
    uint32_t crc = getCrc();

    while (bitIt->bitType == BIT_TYPE_CRC)
    {
        // CRC should be set in CAN FD frames before stuff bits in CRC are
        // inserted (as CRC affects value of these stuff bits), therefore
        // it is illegal to calculate CRC when stuff bits in it are already
        // inserted!
        assert(bitIt->stuffBitType == STUFF_NO);

        bitIt->bitValue = (BitValue)((crc >> i) & 0x1);
        bitIt++;
    }
}

uint32_t can::BitFrame::getBaseIdentifier()
{
    if (frameFlags_.isIde_ == EXTENDED_IDENTIFIER)
        return ((uint32_t)(getIdentifier() >> 18));
    else
        return (uint32_t)getIdentifier();
}

uint32_t can::BitFrame::getIdentifierExtension()
{
    if (frameFlags_.isIde_ == EXTENDED_IDENTIFIER)
        return (uint32_t)(getIdentifier()) & 0x3FFF;
    else
        return 0;
}

uint8_t can::BitFrame::getStuffCount()
{
    if (frameFlags_.isFdf_ == CAN_2_0){
        std::cout << "CAN 2.0 frame does not have Stuff count field defined" << std::endl;
        return 0;
    }
    return stuffCount_;
}

uint32_t can::BitFrame::getCrc()
{
    if (frameFlags_.isFdf_ == CAN_FD)
        return crc15_;
    if (dataLenght_ > 16)
        return crc21_;
    return crc17_;
}

// LSB represents bit value we want to push
void can::BitFrame::push_bit(uint8_t bit_val, BitType bitType)
{
    if ((bit_val % 2) == 0)
        bits_.push_back(Bit(bitType, DOMINANT));
    else
        bits_.push_back(Bit(bitType, RECESSIVE));
}

void can::BitFrame::clearFrameBits()
{
    bits_.clear();
}

bool can::BitFrame::clearFrameBits(int index)
{
    int i = 0;
    std::list<Bit>::iterator bitIt;
    std::list<Bit>::iterator endIt;

    if (index >= bits_.size())
        return false;

    bitIt = bits_.begin();
    endIt = bits_.end();
    std::advance(bitIt, index);
    bits_.erase(bitIt, endIt);
}

void can::BitFrame::buildFrameBits()
{
    clearFrameBits();
    bits_.push_back(Bit(BIT_TYPE_SOF, DOMINANT));

    // Build base ID
    uint32_t baseId = getBaseIdentifier();
    for (int i = 11; i > 0; i--)
        push_bit(baseId >> i, BIT_TYPE_BASE_ID);

    // Build RTR/r1/SRR
    if (frameFlags_.isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(Bit(BIT_TYPE_SRR, RECESSIVE));
    } else {
        if (frameFlags_.isFdf_ == CAN_FD) {
            bits_.push_back(Bit(BIT_TYPE_R1, DOMINANT));
        } else {
            if (frameFlags_.isRtr_ == RTR_FRAME)
                bits_.push_back(Bit(BIT_TYPE_RTR, RECESSIVE));
            else
                bits_.push_back(Bit(BIT_TYPE_RTR, DOMINANT));
        }
    }

    // Build IDE, Extended Identifier and one bit post Extended Identifier
    if (frameFlags_.isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(Bit(BIT_TYPE_IDE, DOMINANT));

        uint32_t extId = getIdentifierExtension();
        for (int i = 18; i > 0; i--)
            push_bit(extId >> i, BIT_TYPE_EXTENDED_ID);

        if (frameFlags_.isFdf_ == CAN_FD) {
            bits_.push_back(Bit(BIT_TYPE_R1, DOMINANT));
        } else {
            if (frameFlags_.isRtr_ == RTR_FRAME) {
                bits_.push_back(Bit(BIT_TYPE_RTR, RECESSIVE));
            } else {
                bits_.push_back(Bit(BIT_TYPE_RTR, DOMINANT));
            }
        }
    } else {
        bits_.push_back(Bit(BIT_TYPE_IDE, RECESSIVE));
    }

    // Build EDL/r0/r1 bit
    if (frameFlags_.isFdf_ == CAN_FD) {
        bits_.push_back(Bit(BIT_TYPE_EDL, RECESSIVE));
    } else if (frameFlags_.isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(Bit(BIT_TYPE_R1, DOMINANT));
    } else {
        bits_.push_back(Bit(BIT_TYPE_R0, DOMINANT));
    }

    // Build extra r0 past EDL or in Extended Identifier frame
    if (frameFlags_.isFdf_ == CAN_FD || frameFlags_.isIde_ == EXTENDED_IDENTIFIER) {
        bits_.push_back(Bit(BIT_TYPE_R0, DOMINANT));
    }

    // Build BRS and ESI bits
    if (frameFlags_.isFdf_ == CAN_FD) {
        if (frameFlags_.isBrs_ == BIT_RATE_SHIFT)
            bits_.push_back(Bit(BIT_TYPE_BRS, RECESSIVE));
        else
            bits_.push_back(Bit(BIT_TYPE_BRS, DOMINANT));
        
        if (frameFlags_.isEsi_ == ESI_ERROR_ACTIVE)
            bits_.push_back(Bit(BIT_TYPE_ESI, DOMINANT));
        else
            bits_.push_back(Bit(BIT_TYPE_ESI, RECESSIVE));
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
    if (frameFlags_.isFdf_ == CAN_FD)
    {
        for (int i = 0; i < 3; i++)
            bits_.push_back(Bit(BIT_TYPE_STUFF_COUNT, DOMINANT));
        bits_.push_back(Bit(BIT_TYPE_STUFF_PARITY, RECESSIVE));
    }

    // Build CRC - put dummies so far since we don't have Stuff bits
    // yet, we can't calculate value of CRC for CAN FD frames!
    int crcLength;

    if (frameFlags_.isFdf_ == CAN_2_0)
        crcLength = 15;
    else if (getDataLenght() <= 16)
        crcLength = 17;
    else
        crcLength = 21;

    for (int i = crcLength - 1; i >= 0; i--)
        push_bit(RECESSIVE, BIT_TYPE_CRC);

    // Add CRC Delimiter, ACK and ACK Delimiter
    bits_.push_back(Bit(BIT_TYPE_CRC_DELIMITER, RECESSIVE));
    bits_.push_back(Bit(BIT_TYPE_ACK, RECESSIVE));
    bits_.push_back(Bit(BIT_TYPE_ACK_DELIMITER, RECESSIVE));

    // Finalize by EOF and by Intermission
    for (int i = 0; i < 7; i++)
        bits_.push_back(Bit(BIT_TYPE_EOF, RECESSIVE));
    for (int i = 0; i < 3; i++)
        bits_.push_back(Bit(BIT_TYPE_INTERMISSION, RECESSIVE));
}

int can::BitFrame::insertNormalStuffBits()
{
    std::list<Bit>::iterator bitIt;
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
    for (bitIt = ++(bits_.begin());; ++bitIt)
    {
        // Break when we reach Stuff count (CAN FD) or CRC Delimiter (CAN 2.0).
        // Account also improperly created frame so break on the end!
        if (bitIt->bitType == BIT_TYPE_CRC_DELIMITER ||
            bitIt->bitType == BIT_TYPE_STUFF_COUNT ||
            bitIt == bits_.end())
            break;

        if (bitIt->bitValue == prevValue)
            sameBits++;
        else
            sameBits = 1;

        if (sameBits == 5)
        {
            BitValue stuffValue = bitIt->getOppositeValue();
            BitType stuffBitType = bitIt->bitType;
            bitIt++;
            bitIt = bits_.insert(bitIt, Bit(stuffBitType, stuffValue));
            bitIt->stuffBitType = STUFF_NORMAL;
            sameBits = 1;

            stuffCount_ = (stuffCount_ + 1) % 8;
        }
        prevValue = bitIt->bitValue;
    }

    return stuffCount_;
}

bool can::BitFrame::insertStuffCountStuffBits()
{
    std::list<Bit>::iterator bitIt;
    BitValue stuffBitValue;

    if (frameFlags_.isFdf_ == CAN_2_0)
        return false;

    for (bitIt = bits_.begin(); bitIt->bitType != BIT_TYPE_STUFF_COUNT; bitIt++)
        ;
    bitIt--;
    stuffBitValue = bitIt->getOppositeValue();
    bitIt++;

    bitIt = bits_.insert(bitIt, Bit(BIT_TYPE_STUFF_COUNT, stuffBitValue)); 
    bitIt->stuffBitType = STUFF_FIXED;
    bitIt++;

    // Move one beyond stuff parity and calculate stuff bit post parity
    for (int i = 0; i < 3; i++)
        bitIt++;
    stuffBitValue = bitIt->getOppositeValue();
    bitIt->getOppositeValue();
    bitIt++;

    bitIt = bits_.insert(bitIt, Bit(BIT_TYPE_STUFF_PARITY, stuffBitValue));
    bitIt->stuffBitType = STUFF_FIXED;

    return true;
}

void can::BitFrame::insertCrcFixedStuffBits()
{
    std::list<Bit>::iterator bitIt;
    int sameBits = 0;

    // Search first bit of CRC 
    for (bitIt = bits_.begin(); bitIt->bitType != BIT_TYPE_CRC; bitIt++)
        ;

    for (; bitIt->bitType != BIT_TYPE_CRC_DELIMITER; ++bitIt)
    {
        sameBits++;
        if ((sameBits % 4) == 0)
        {

            // Second fixed stuff bit is still in Stuff count field!
            // This is important for CRC calculation!
            BitType bitType;
            BitValue bitValue;
            
            bitType = BIT_TYPE_CRC;
            bitValue = bitIt->getOppositeValue();
            bitIt = bits_.insert(++bitIt, Bit(bitType, bitValue));
            bitIt->stuffBitType = STUFF_FIXED;
        }
    }

}

uint32_t can::BitFrame::calculateCrc()
{
    std::list<Bit>::iterator bitIt;
    uint32_t crcNxt15 = 0;
    uint32_t crcNxt17 = 0;
    uint32_t crcNxt21 = 0;

    crc15_ = 0x0;
    crc17_ = (1 << 17);
    crc21_ = (1 << 21);

    // CRC calculation as in CAN FD spec!
    bitIt = bits_.begin();
    while (true)
    {
        if (bitIt->bitType == BIT_TYPE_CRC)
            break;

        crcNxt15 = (int)(bitIt->bitValue) ^ ((crc15_ >> 15) & 0x1);
        crcNxt17 = (int)(bitIt->bitValue) ^ ((crc17_ >> 17) & 0x1);
        crcNxt21 = (int)(bitIt->bitValue) ^ ((crc15_ >> 21) & 0x1);

        // Shift left, CRC 15 always without stuff bits
        if (bitIt->stuffBitType == STUFF_NO)
            crc15_ = (crc15_ << 1) | ((uint32_t)(bitIt->bitValue) & 0x1);

        crc17_ = (crc17_ << 1) | ((uint32_t)(bitIt->bitValue) & 0x1);
        crc21_ = (crc21_ << 1) | ((uint32_t)(bitIt->bitValue) & 0x1);

        // Calculate by polynomial
        if (crcNxt15 == 1 && bitIt->stuffBitType == STUFF_NO)
            crc15_ ^= 0xC599;
        if (crcNxt17 == 1)
            crc17_ ^= 0x3685B;
        if (crcNxt21 == 1)
            crc21_ ^= 0x302899;

        bitIt++;
    }

    if (frameFlags_.isFdf_ == CAN_2_0)
        return crc15_;
    if (dataLenght_ <= 16)
        return crc17_;
    return crc21_;
}

bool can::BitFrame::setStuffCount()
{
    std::list<Bit>::iterator bitIt;
    stuffCountEncoded_ = 0;
    bitIt = bits_.begin();

    // No sense to try to set Stuff count on CAN 2.0 frames!
    if (frameFlags_.isFdf_ == CAN_2_0)
        return false;

    while (bitIt->bitType != BIT_TYPE_STUFF_COUNT && bitIt != bits_.end())
        bitIt++;

    if (bitIt == bits_.end())
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
        assert(bitIt->bitType == BIT_TYPE_STUFF_COUNT);
        bitIt->bitValue = (BitValue)((stuffCountEncoded_ >> i) & 0x1);
        bitIt++;
    }
    return true;
}

bool can::BitFrame::setStuffParity()
{
    std::list<Bit>::iterator bitIt;
    uint8_t val = 0;

    if (frameFlags_.isFdf_ == CAN_2_0)
        return false;

    for (bitIt = bits_.begin(); bitIt->bitType != BIT_TYPE_STUFF_PARITY; bitIt++)
        ;
    for (int i = 0; i < 3; i++)
        val ^= (stuffCountEncoded_ >> i) & 0x1;
    bitIt->setBitValue((BitValue)val);

    // We must correct value of Fixed stuff bit after the stuff parity!
    // TODO:

    return true;
}

can::Bit* can::BitFrame::getBit(int index)
{
    std::list<Bit>::iterator bitIt = bits_.begin();

    if (bits_.size() <= index)
        return nullptr;

    std::advance(bitIt, index);
    return &(*bitIt);
}

std::list<can::Bit>::iterator can::BitFrame::getBitIterator(int index)
{
    std::list<Bit>::iterator bitIt = bits_.begin();

    if (bits_.size() <= index)
        return bitIt;

    std::advance(bitIt, index);
    return bitIt;
}

can::Bit* can::BitFrame::getBitOf(int index, BitType bitType)
{
    std::list<Bit>::iterator bitIt = bits_.begin();
    int i = 0;

    while (bitIt != bits_.end())
    {
        if (bitIt->bitType == bitType)
            if (i == index) {
                break;
            } else {
                i++;
                bitIt++;
            }
        else
            bitIt++;
    }

    if (bitIt == bits_.end())
        return nullptr;

    return &(*bitIt);
}

std::list<can::Bit>::iterator can::BitFrame::getBitOfIterator(int index, BitType bitType)
{
    std::list<Bit>::iterator bitIt = bits_.begin();
    int i = 0;

    while (bitIt != bits_.end())
    {
        if (bitIt->bitType == bitType)
            if (i == index) {
                break;
            } else {
                i++;
                bitIt++;
            }
        else
            bitIt++;
    }

    return bitIt;
}

int can::BitFrame::getBitIndex(Bit *bit)
{
    std::list<Bit>::iterator bitIt = bits_.begin();
    int i = 0;

    while (&(*bitIt) != bit && bitIt != bits_.end()) {
        i++;
        bitIt++;
    }
    return i;
}

can::Bit* can::BitFrame::getStuffBit(int index)
{
    std::list<Bit>::iterator bitIt = bits_.begin();
    int i = 0;

    while (i < index || bitIt != bits_.end())
        if (bitIt->stuffBitType == STUFF_NORMAL){
            i++;
            bitIt++;
        }

    if (bitIt == bits_.end())
        return nullptr;

    return &(*bitIt);
}

can::Bit* can::BitFrame::getFixedStuffBit(int index)
{
    std::list<Bit>::iterator bitIt = bits_.begin();
    int i = 0;

    while (i < index || bitIt != bits_.end())
        if (bitIt->stuffBitType == STUFF_FIXED){
            i++;
            bitIt++;
        }

    if (bitIt == bits_.end())
        return nullptr;

    return &(*bitIt);
}

bool can::BitFrame::insertBit(Bit bit, int index)
{
    if (index >= bits_.size())
        return false;

    std::list<Bit>::iterator bitIt;
    std::advance(bitIt, index);
    bits_.insert(bitIt, bit);

    return true;
}

bool can::BitFrame::removeBit(Bit *bit)
{
    std::list<Bit>::iterator bitIt = bits_.begin();
    while (bitIt != bits_.end())
    {
        if (&(*bitIt) == bit)
            break;
        bitIt++;
    };
    bits_.erase(bitIt);
}

bool can::BitFrame::removeBit(int index)
{
    std::list<Bit>::iterator bitIt;

    if (bits_.size() <= index)
        return false;

    std::advance(bitIt, index);
    bits_.erase(bitIt);

    return true;
}

bool can::BitFrame::insertAck()
{
    // This assumes only first ACK bit is set. In case we have more of them
    // (like prolonged ACK in CAN FD frame, we do only the first one)    
    Bit *bit = getBitOf(0, BIT_TYPE_ACK);

    if (bit == nullptr)
        return false;
    
    bit->setBitValue(DOMINANT);
    return true;
}

bool can::BitFrame::insertActiveErrorFrame(int index)
{
    Bit *bit = getBit(index);

    if (bit == nullptr)
        return false;

    // Discard all bits from this bit further
    clearFrameBits(index);

    // Insert Active Error flag and Error delimiter
    for (int i = 0; i < 6; i++)
        bits_.push_back(Bit(BIT_TYPE_ACTIVE_ERROR_FLAG, DOMINANT));
    for (int i = 0; i < 8; i++)
        bits_.push_back(Bit(BIT_TYPE_ERROR_DELIMITER, RECESSIVE));

    return true;
}

bool can::BitFrame::insertActiveErrorFrame(Bit *bit)
{
    return insertActiveErrorFrame(getBitIndex(bit));
}

bool can::BitFrame::insertPassiveErrorFrame(int index)
{
    Bit *bit = getBit(index);

    if (bit == nullptr)
        return false;

    // Discard all bits from this bit further
    clearFrameBits(index);

    for (int i = 0; i < 6; i++)
        bits_.push_back(Bit(BIT_TYPE_PASSIVE_ERROR_FLAG, RECESSIVE));
    for (int i = 0; i < 8; i++)
        bits_.push_back(Bit(BIT_TYPE_ERROR_DELIMITER, RECESSIVE));

    return true;
}

bool can::BitFrame::insertPassiveErrorFrame(Bit *bit)
{
    return insertPassiveErrorFrame(getBitIndex(bit));
}

bool can::BitFrame::insertOverloadFrame(int index)
{
    Bit *bit = getBit(index);

    if (bit == nullptr)
        return false;

    if (bit->bitType != BIT_TYPE_INTERMISSION &&
        bit->bitType != BIT_TYPE_ERROR_DELIMITER &&
        bit->bitType != BIT_TYPE_OVERLOAD_DELIMITER)
    {
        std::cerr << " Can't insert Overload frame on " << bit->bitType <<
            std::endl;
        return false;
    }

    for (int i = 0; i < 6; i++)
        bits_.push_back(Bit(BIT_TYPE_OVERLOAD_FLAG, DOMINANT));
    for (int i = 0; i < 8; i++)
        bits_.push_back(Bit(BIT_TYPE_OVERLOAD_DELIMITER, RECESSIVE));
    return true;
}

bool can::BitFrame::insertOverloadFrame(Bit *bit)
{
    return insertOverloadFrame(getBitIndex(bit));
}

bool can::BitFrame::looseArbitration(int index)
{
    Bit *bit = getBit(index);
    std::list<Bit>::iterator bitIt = bits_.begin();

    if (bit == nullptr)
        return false;

    if (bit->bitType != BIT_TYPE_BASE_ID &&
        bit->bitType != BIT_TYPE_EXTENDED_ID &&
        bit->bitType != BIT_TYPE_RTR &&
        bit->bitType != BIT_TYPE_SRR &&
        bit->bitType != BIT_TYPE_IDE)
    {
        std::cerr << "Can't loose arbitration on " << bit->bitType << std::endl;
        return false;
    }

    // Move to position where we want to loose arbitration
    for (int i = 0; i < index; i++)
        bitIt++;

    for (; bitIt != bits_.end(); bitIt++){
        if (bitIt->bitType == BIT_TYPE_ACK)
            bitIt->bitValue = DOMINANT;
        else
            bitIt->bitValue = RECESSIVE;
    }
    return true;
}

bool can::BitFrame::looseArbitration(Bit *bit)
{
    return looseArbitration(getBitIndex(bit));
}


void can::BitFrame::print(bool printStuffBits)
{
    std::list<Bit>::iterator bitIt;

    std::string vals = "";
    std::string names = "";

    for (bitIt = bits_.begin(); bitIt != bits_.end();)
    {
        // Print separators betwen different field types (also prints separator
        //  at start of frame)
        vals += "|";
        names += " ";

        // Both methods advance iterator when bit is printed.
        if (bitIt->isSingleBitField()) {
            //if (printStuffBits == false && bit->stuffBitType != STUFF_NO)
            //    continue;
            printSingleBitField(bitIt, &vals, &names, printStuffBits);
        } else {
            printMultiBitField(bitIt, &vals, &names, printStuffBits);
        }
    }

    std::cout << names << std::endl;
    std::cout << std::string(names.length(), '-') << std::endl;
    std::cout << vals << std::endl;
    std::cout << std::string(names.length(), '-') << std::endl;
}



void can::BitFrame::printSingleBitField(std::list<Bit>::iterator& bitIt,
                                        std::string *vals,
                                        std::string *names,
                                        bool printStuffBits)
{
    std::list<Bit>::iterator nxtBitIt;
    nxtBitIt = bitIt;
    nxtBitIt++;

    // Print the bit itself
    vals->append(" " + bitIt->getStringValue() + " ");
    names->append(bitIt->getBitTypeName());
    bitIt++;

    // Handle stuff bit. If stuff bit is inserted behind a single bit
    // field it is marked with the same bit field!
    if (nxtBitIt->bitType == bitIt->bitType &&
        (nxtBitIt->stuffBitType == STUFF_FIXED ||
         nxtBitIt->stuffBitType == STUFF_NORMAL))
    {
        if (printStuffBits == true)
        {
            names->append(std::string(3, ' '));
            vals->append(" " + bitIt->getStringValue() + " ");
        }
        bitIt++;
    }
}

void can::BitFrame::printMultiBitField(std::list<Bit>::iterator& bitIt,
                                       std::string *vals,
                                       std::string *names,
                                       bool printStuffBits)
{
    int len = 0;
    int preOffset = 0;
    int postOffset = 0;
    std::string fieldName = bitIt->getBitTypeName();
    std::list<Bit>::iterator firstBitIt = bitIt;

    for (; bitIt->bitType == firstBitIt->bitType; bitIt++)
    {
        if (printStuffBits == false && bitIt->stuffBitType != STUFF_NO)
            continue;

        len += 2;
        vals->append(bitIt->getStringValue() + " ");
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

    names->append(std::string(preOffset, ' '));
    names->append(fieldName);
    names->append(std::string(postOffset, ' '));
}