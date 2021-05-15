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
#include <assert.h>

#include "can.h"
#include "Bit.h"
#include "BitFrame.h"

void can::BitFrame::ConstructFrame()
{
    BuildFrameBits();

    if (frame_flags().is_fdf_ == FrameType::Can2_0){
        CalculateCrc();
        UpdateCrcBits();

        // We must set CRC before Stuff bits are inserted because in CAN 2.0
        // frames regular stuff bits are inserted also to CRC!
        InsertNormalStuffBits();
    } else {
        InsertNormalStuffBits();
        SetStuffCount();
        SetStuffParity();
        InsertStuffCountStuffBits();
        CalculateCrc();
        UpdateCrcBits();
        InsertCrcFixedStuffBits();
    }
}


can::BitFrame::BitFrame(FrameFlags frame_flags, uint8_t dlc, int identifier,
                        uint8_t *data, BitTiming* nominal_bit_timing,
                        BitTiming* data_bit_timing):
                Frame(frame_flags, dlc, identifier, data)
{
    nominal_bit_timing_ = nominal_bit_timing;
    data_bit_timing_ = data_bit_timing;

    ConstructFrame();
}


can::BitFrame::BitFrame(Frame &frame, BitTiming *nominal_bit_timing,
                        BitTiming *data_bit_timing):
                Frame(frame.frame_flags(), frame.dlc(),
                      frame.identifier(), frame.data())
{
    nominal_bit_timing_ = nominal_bit_timing;
    data_bit_timing_ = data_bit_timing;

    ConstructFrame();
}


void can::BitFrame::UpdateCrcBits()
{
    std::list<Bit>::iterator bit_it = GetBitOfIterator(0, BitType::Crc);
    uint32_t tmp_crc = crc();
    int i;

    if (frame_flags_.is_fdf_ == FrameType::Can2_0)
        i = 14;
    else if (data_lenght_ > 16)
        i = 20;
    else
        i = 16;

    while (bit_it->bit_type_ == BitType::Crc)
    {
        // CRC should be set in CAN FD frames before stuff bits in CRC are
        // inserted (as CRC affects value of these stuff bits), therefore
        // it is illegal to calculate CRC when stuff bits in it are already
        // inserted!
        assert(bit_it->stuff_bit_type == StuffBitType::NoStuffBit);

        bit_it->bit_value_ = (BitValue)((tmp_crc >> i) & 0x1);
        i--;
        bit_it++;
    }
}


uint32_t can::BitFrame::base_identifier()
{
    if (frame_flags_.is_ide_ == IdentifierType::Extended)
        return ((uint32_t)(identifier() >> 18));
    else
        return (uint32_t)identifier();
}


uint32_t can::BitFrame::identifier_extension()
{
    if (frame_flags_.is_ide_ == IdentifierType::Extended)
        return (uint32_t)(identifier()) & 0x3FFFF;
    else
        return 0;
}


uint8_t can::BitFrame::stuff_count()
{
    if (frame_flags_.is_fdf_ == FrameType::Can2_0){
        std::cerr << "CAN 2.0 frame does not have Stuff count field defined" << std::endl;
        return 0;
    }
    return stuff_count_;
}


uint32_t can::BitFrame::crc()
{
    if (frame_flags_.is_fdf_ == FrameType::Can2_0)
        return crc15_;
    if (data_lenght_ > 16)
        return crc21_;
    return crc17_;
}


// LSB represents bit value we want to push
void can::BitFrame::AppendBit(BitType bit_type, uint8_t bit_val)
{
    BitValue bit_value;

    if ((bit_val % 2) == 0)
        bit_value = BitValue::Dominant;
    else
        bit_value = BitValue::Recessive;

    AppendBit(bit_type, bit_value);
}


void can::BitFrame::AppendBit(BitType bit_type, BitValue bit_value)
{
    bits_.push_back(Bit(this, bit_type, bit_value, &frame_flags_, nominal_bit_timing_,
                        data_bit_timing_));
}


void can::BitFrame::AppendBitFrame(can::BitFrame *bit_frame)
{
    for (size_t i = 0; i < bit_frame->GetBitCount(); i++)
        // We want to copy the bit, not to refer to original bit frame!
        bits_.push_back(Bit(*bit_frame->GetBit(i)));
}


bool can::BitFrame::ClearFrameBits(size_t index)
{
    std::list<Bit>::iterator bit_it;
    std::list<Bit>::iterator end_it;

    if (index >= bits_.size())
        return false;

    bit_it = bits_.begin();
    end_it = bits_.end();
    std::advance(bit_it, index);
    bits_.erase(bit_it, end_it);

    return true;
}

void can::BitFrame::BuildFrameBits()
{
    bits_.clear();
    AppendBit(BitType::Sof, BitValue::Dominant);

    // Build base ID
    uint32_t base_id = base_identifier();
    for (int i = 10; i >= 0; i--)
        AppendBit(BitType::BaseIdentifier, base_id >> i);

    // Build RTR/r1/SRR
    if (frame_flags_.is_ide_ == IdentifierType::Extended) {
        AppendBit(BitType::Srr, BitValue::Recessive);
    } else {
        if (frame_flags_.is_fdf_ == FrameType::CanFd) {
            AppendBit(BitType::R1, BitValue::Dominant);
        } else {
            if (frame_flags_.is_rtr_ == RtrFlag::RtrFrame)
                AppendBit(BitType::Rtr, BitValue::Recessive);
            else
                AppendBit(BitType::Rtr, BitValue::Dominant);
        }
    }

    // Build IDE, Extended Identifier and one bit post Extended Identifier
    if (frame_flags_.is_ide_ == IdentifierType::Extended) {
        AppendBit(BitType::Ide, BitValue::Recessive);

        uint32_t extId = identifier_extension();
        for (int i = 17; i >= 0; i--)
            AppendBit(BitType::IdentifierExtension, extId >> i);

        if (frame_flags_.is_fdf_ == FrameType::CanFd) {
            AppendBit(BitType::R1, BitValue::Dominant);
        } else {
            if (frame_flags_.is_rtr_ == RtrFlag::RtrFrame) {
                AppendBit(BitType::Rtr, BitValue::Recessive);
            } else {
                AppendBit(BitType::Rtr, BitValue::Dominant);
            }
        }
    } else {
        AppendBit(BitType::Ide, BitValue::Dominant);
    }

    // Build EDL/r0/r1 bit
    if (frame_flags_.is_fdf_ == FrameType::CanFd) {
        AppendBit(BitType::Edl, BitValue::Recessive);
    } else if (frame_flags_.is_ide_ == IdentifierType::Extended) {
        AppendBit(BitType::R1, BitValue::Dominant);
    } else {
        AppendBit(BitType::R0, BitValue::Dominant);
    }

    // Build extra r0 past EDL or in Extended Identifier frame
    if (frame_flags_.is_fdf_ == FrameType::CanFd || frame_flags_.is_ide_ == IdentifierType::Extended) {
        AppendBit(BitType::R0, BitValue::Dominant);
    }

    // Build BRS and ESI bits
    if (frame_flags_.is_fdf_ == FrameType::CanFd) {
        if (frame_flags_.is_brs_ == BrsFlag::Shift)
            AppendBit(BitType::Brs, BitValue::Recessive);
        else
            AppendBit(BitType::Brs, BitValue::Dominant);
        
        if (frame_flags_.is_esi_ == EsiFlag::ErrorActive)
            AppendBit(BitType::Esi, BitValue::Dominant);
        else
            AppendBit(BitType::Esi, BitValue::Recessive);
    }

    // Build DLC
    for (int i = 3; i >= 0; i--)
        AppendBit(BitType::Dlc, dlc_ >> i);

    // Build data field
    for (int i = 0; i < data_length(); i++) {
        for (int j = 7; j >= 0; j--)
            AppendBit(BitType::Data, data(i) >> j);
    }
    
    // Build Stuff count + parity (put dummy as we don't know number of
    // stuff bits yet)!
    if (frame_flags_.is_fdf_ == FrameType::CanFd)
    {
        for (int i = 0; i < 3; i++)
            AppendBit(BitType::StuffCount, BitValue::Dominant);
        AppendBit(BitType::StuffParity, BitValue::Recessive);
    }

    // Build CRC - put dummies so far since we don't have Stuff bits
    // yet, we can't calculate value of CRC for CAN FD frames!
    int crc_length;

    if (frame_flags_.is_fdf_ == FrameType::Can2_0)
        crc_length = 15;
    else if (data_length() <= 16)
        crc_length = 17;
    else
        crc_length = 21;

    for (int i = crc_length - 1; i >= 0; i--)
        AppendBit(BitType::Crc, BitValue::Recessive);

    // Add CRC Delimiter, ACK and ACK Delimiter
    AppendBit(BitType::CrcDelimiter, BitValue::Recessive);
    AppendBit(BitType::Ack, BitValue::Recessive);
    if (frame_flags_.is_fdf_ == FrameType::CanFd)
        AppendBit(BitType::Ack, BitValue::Recessive);
    AppendBit(BitType::AckDelimiter, BitValue::Recessive);

    // Finalize by EOF and by Intermission
    for (int i = 0; i < 7; i++)
        AppendBit(BitType::Eof, BitValue::Recessive);
    for (int i = 0; i < 3; i++)
        AppendBit(BitType::Intermission, BitValue::Recessive);
}


size_t can::BitFrame::InsertNormalStuffBits()
{
    std::list<Bit>::iterator bit_it;
    int same_bits = 1;
    stuff_count_ = 0;
    BitValue prev_value = BitValue::Dominant; // As if SOF

    if (bits_.front().bit_type_ != BitType::Sof) {
        std::cerr << "First bit of a frame should be SOF!" << std::endl;
        return 0;
    }

    if (bits_.size() < 5) {
        std::cerr << "At least 5 bits needed for bit stuffing!" << std::endl;
    }

    // Start from first bit of Base identifier
    for (bit_it = ++(bits_.begin());; ++bit_it)
    {
        // Break when we reach Stuff count (CAN FD) or CRC Delimiter (CAN 2.0).
        // Account also improperly created frame so break on the end!
        if (bit_it->bit_type_ == BitType::CrcDelimiter ||
            bit_it->bit_type_ == BitType::StuffCount ||
            bit_it == bits_.end())
            break;

        if (bit_it->bit_value_ == prev_value)
            same_bits++;
        else
            same_bits = 1;

        if (same_bits == 5)
        {
            // This is exception for stuff bit inserted just before Stuff count!
            // There shall be no regular stuff bit inserted before stuff count
            // even if there are 5 consecutive bits of equal value. This bit shall
            // not be taken into number of stuffed bits!
            auto bit_it_nxt = bit_it;
            bit_it_nxt++;
            if (bit_it_nxt->bit_type_ == BitType::StuffCount)
            {
                prev_value = bit_it->bit_value_;
                continue;
            }

            Bit bit = Bit(this, bit_it->bit_type_, bit_it->GetOppositeValue(),
                          &frame_flags_, nominal_bit_timing_, data_bit_timing_,
                          StuffBitType::NormalStuffBit);
            bit_it++;
            bit_it = bits_.insert(bit_it, bit);
            same_bits = 1;

            stuff_count_ = (stuff_count_ + 1) % 8;
        }
        prev_value = bit_it->bit_value_;
    }

    return stuff_count_;
}


void can::BitFrame::InsertStuffCountStuffBits()
{
    std::list<Bit>::iterator bit_it;
    BitValue stuff_bit_value;

    assert(!(frame_flags_.is_fdf_ == FrameType::Can2_0));

    for (bit_it = bits_.begin(); bit_it->bit_type_ != BitType::StuffCount; bit_it++)
        ;
    bit_it--;
    stuff_bit_value = bit_it->GetOppositeValue();
    bit_it++;

    Bit bit = Bit(this, BitType::StuffCount, stuff_bit_value, &frame_flags_,
                  nominal_bit_timing_, data_bit_timing_, StuffBitType::FixedStuffBit);
    bit_it = bits_.insert(bit_it, bit);
    bit_it++;

    // Move one beyond stuff parity and calculate stuff bit post parity
    for (int i = 0; i < 3; i++)
        bit_it++;
    stuff_bit_value = bit_it->GetOppositeValue();
    bit_it->GetOppositeValue();
    bit_it++;

    Bit bit_2 = Bit(this, BitType::StuffParity, stuff_bit_value, &frame_flags_,
                    nominal_bit_timing_, data_bit_timing_, StuffBitType::FixedStuffBit);
    bit_it = bits_.insert(bit_it, bit_2);
}


void can::BitFrame::InsertCrcFixedStuffBits()
{
    std::list<Bit>::iterator bit_it;
    int same_bits = 0;

    // Search first bit of CRC 
    for (bit_it = bits_.begin(); bit_it->bit_type_ != BitType::Crc; bit_it++)
        ;

    for (; bit_it->bit_type_ != BitType::CrcDelimiter; ++bit_it)
    {
        same_bits++;
        if ((same_bits % 4) == 0)
        {
            Bit bit = Bit(this, BitType::Crc, bit_it->GetOppositeValue(),
                          &frame_flags_, nominal_bit_timing_, data_bit_timing_,
                          StuffBitType::FixedStuffBit);
            bit_it = bits_.insert(++bit_it, bit);
        }
    }

}


uint32_t can::BitFrame::CalculateCrc()
{
    std::list<Bit>::iterator bit_it;
    uint32_t crc_nxt_15 = 0;
    uint32_t crc_nxt_17 = 0;
    uint32_t crc_nxt_21 = 0;

    crc15_ = 0;
    crc17_ = (1 << 16);
    crc21_ = (1 << 20);

    // CRC calculation as in CAN FD spec!
    bit_it = bits_.begin();
    while (true)
    {
        if (bit_it->bit_type_ == BitType::Crc)
            break;

        BitValue bit_value = bit_it->bit_value_;
        crc_nxt_15 = (uint32_t)(bit_value) ^ ((crc15_ >> 14) & 0x1);
        crc_nxt_17 = (uint32_t)(bit_value) ^ ((crc17_ >> 16) & 0x1);
        crc_nxt_21 = (uint32_t)(bit_value) ^ ((crc21_ >> 20) & 0x1);

        // Shift left, CRC 15 always without stuff bits
        if (bit_it->stuff_bit_type == StuffBitType::NoStuffBit)
            crc15_ = (crc15_ << 1);

        if (bit_it->stuff_bit_type != StuffBitType::FixedStuffBit)
        {
            crc17_ = (crc17_ << 1);
            crc21_ = (crc21_ << 1);
        }

        crc15_ &= 0x7FFF;
        crc17_ &= 0x1FFFF;
        crc21_ &= 0x1FFFFF;

        // Calculate by polynomial
        if ((crc_nxt_15 == 1) && (bit_it->stuff_bit_type == StuffBitType::NoStuffBit))
            crc15_ ^= 0xC599;
        if ((crc_nxt_17 == 1) && (bit_it->stuff_bit_type != StuffBitType::FixedStuffBit))
            crc17_ ^= 0x3685B;
        if ((crc_nxt_21 == 1) && (bit_it->stuff_bit_type != StuffBitType::FixedStuffBit))
            crc21_ ^= 0x302899;

        bit_it++;
    }

    //printf("Calculated CRC 15 : 0x%x\n", crc15_);
    //printf("Calculated CRC 17 : 0x%x\n", crc17_);
    //printf("Calculated CRC 21 : 0x%x\n", crc21_);

    if (frame_flags_.is_fdf_ == FrameType::Can2_0)
        return crc15_;
    else if (data_lenght_ <= 16)
        return crc17_;
    else
        return crc21_;
}


bool can::BitFrame::SetStuffCount()
{
    std::list<Bit>::iterator bit_it;
    stuff_count_encoded_ = 0;
    bit_it = bits_.begin();

    // DontShift sense to try to set Stuff count on CAN 2.0 frames!
    if (frame_flags_.is_fdf_ == FrameType::Can2_0)
        return false;

    while (bit_it->bit_type_ != BitType::StuffCount && bit_it != bits_.end())
        bit_it++;

    if (bit_it == bits_.end())
    {
        std::cerr << "Did not find stuff count field!" << std::endl;
        return false;
    }

    assert(stuff_count_ < 8);

    switch (stuff_count_){
        case 0x0 :
            stuff_count_encoded_ = 0b000;
            break;
        case 0x1:
            stuff_count_encoded_ = 0b001;
            break;
        case 0x2 :
            stuff_count_encoded_ = 0b011;
            break;
        case 0x3 :
            stuff_count_encoded_ = 0b010;
            break;
        case 0x4 :
            stuff_count_encoded_ = 0b110;
            break;
        case 0x5 :
            stuff_count_encoded_ = 0b111;
            break;
        case 0x6 :
            stuff_count_encoded_ = 0b101;
            break;
        case 0x7 :
            stuff_count_encoded_ = 0b100;
            break;
    };

    for (int i = 2; i >= 0; i--)
    {
        assert(bit_it->bit_type_ == BitType::StuffCount);
        bit_it->bit_value_ = (BitValue)((stuff_count_encoded_ >> i) & 0x1);
        bit_it++;
    }
    return true;
}


bool can::BitFrame::SetStuffParity()
{
    std::list<Bit>::iterator bit_it;
    uint8_t val = 0;

    if (frame_flags_.is_fdf_ == FrameType::Can2_0)
        return false;

    for (bit_it = bits_.begin(); bit_it->bit_type_ != BitType::StuffParity; bit_it++)
        ;
    for (int i = 0; i < 3; i++)
        val ^= (stuff_count_encoded_ >> i) & 0x1;
    bit_it->bit_value_ = (BitValue)val;

    return true;
}


size_t can::BitFrame::GetBitCount()
{
    return bits_.size();
}

        
size_t can::BitFrame::GetFieldLength(BitType bit_type)
{
    return std::count_if(bits_.begin(), bits_.end(),
                         [bit_type](can::Bit bit) { return bit.bit_type_ == bit_type; });
}


can::Bit* can::BitFrame::GetRandomBitOf(BitType bit_type)
{
    size_t bit_field_length = GetFieldLength(bit_type);
    assert(bit_field_length > 0 && "Frame has no bits of required type!");

    std::list<Bit>::iterator bit_it =
        std::find_if(bits_.begin(), bits_.end(),
                     [bit_type](Bit bit) { return bit.bit_type_ == bit_type; });

    size_t bit_index = rand() % bit_field_length;
    std::advance(bit_it, bit_index);

    return &(*bit_it);
}

can::Bit* can::BitFrame::GetRandomBit(BitValue bit_value)
{
    Bit *bit;
    int lenght = this->GetBitCount();
    do
    {
        bit = GetBit(rand() % lenght);
    } while (bit->bit_value_ != bit_value);
    return bit;
}

can::Bit* can::BitFrame::GetBit(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();

    assert(bits_.size() > index && "Insufficient number of bits in a frame!");

    std::advance(bit_it, index);
    return &(*bit_it);
}

std::list<can::Bit>::iterator can::BitFrame::GetBitIterator(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();

    assert(bits_.size() > index && "Insufficient number of bits in a frame!");

    std::advance(bit_it, index);
    return bit_it;
}


can::Bit* can::BitFrame::GetBitOf(size_t index, BitType bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (bit_it != bits_.end())
    {
        if (bit_it->bit_type_ == bit_type)
            if (i == index) {
                break;
            } else {
                i++;
                bit_it++;
            }
        else
            bit_it++;
    }

    assert(bit_it != bits_.end() && "Insufficient number of bits in a bit field");

    return &(*bit_it);
}


can::Bit* can::BitFrame::GetBitOfNoStuffBits(size_t index, BitType bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (bit_it != bits_.end())
    {
        if (bit_it->bit_type_ == bit_type &&
            bit_it->stuff_bit_type == StuffBitType::NoStuffBit)
            if (i == index) {
                break;
            } else {
                i++;
                bit_it++;
            }
        else
            bit_it++;
    }

    assert(bit_it != bits_.end() && "Insufficient number of bits in a bit field");

    return &(*bit_it);
}


std::list<can::Bit>::iterator can::BitFrame::GetBitOfIterator(size_t index, BitType bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (bit_it != bits_.end())
    {
        if (bit_it->bit_type_ == bit_type)
            if (i == index) {
                break;
            } else {
                i++;
                bit_it++;
            }
        else
            bit_it++;
    }

    return bit_it;
}


size_t can::BitFrame::GetBitIndex(Bit *bit)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    int i = 0;

    while (&(*bit_it) != bit && bit_it != bits_.end()) {
        i++;
        bit_it++;
    }
    return i;
}


can::Bit* can::BitFrame::GetStuffBit(int index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    int i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if (bit_it->stuff_bit_type == StuffBitType::NormalStuffBit ||
            bit_it->stuff_bit_type == StuffBitType::FixedStuffBit)
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}

can::Bit* can::BitFrame::GetStuffBit(int index, BitType bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    int i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if ((bit_it->stuff_bit_type == StuffBitType::NormalStuffBit ||
             bit_it->stuff_bit_type == StuffBitType::FixedStuffBit) &&
            (bit_it->bit_type_ == bit_type))
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}

can::Bit* can::BitFrame::GetStuffBit(BitType bit_type, StuffBitType stuff_bit_type,
                                     BitValue bit_value)
{
    auto bit_it = std::find_if(bits_.begin(), bits_.end(),
                               [bit_type, stuff_bit_type, bit_value] (Bit bit) {
        if (bit.bit_type_ == bit_type &&
            bit.bit_value_ == bit_value &&
            bit.stuff_bit_type == stuff_bit_type)
            return true;
        return false;
        });
    return &(*bit_it);
}

can::Bit* can::BitFrame::GetFixedStuffBit(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if (bit_it->stuff_bit_type == StuffBitType::FixedStuffBit)
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}

    
can::Bit* can::BitFrame::GetFixedStuffBit(size_t index, BitValue bit_value)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if (bit_it->stuff_bit_type == StuffBitType::FixedStuffBit &&
            bit_it->bit_value_ == bit_value)
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}


bool can::BitFrame::InsertBit(Bit bit, size_t index)
{
    if (index > bits_.size())
        return false;

    std::list<Bit>::iterator bit_it = bits_.begin();
    std::advance(bit_it, index);
    bits_.insert(bit_it, bit);

    return true;
}


bool can::BitFrame::InsertBit(BitType bit_type, BitValue bit_value, size_t index)
{
    return InsertBit(Bit(this, bit_type, bit_value, &frame_flags_, nominal_bit_timing_,
                         data_bit_timing_), index);
}


void can::BitFrame::AppendBit(Bit can_bit)
{
    bits_.push_back(can_bit);
}


void can::BitFrame::RemoveBit(Bit *bit)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    while (bit_it != bits_.end())
    {
        if (&(*bit_it) == bit)
            break;
        bit_it++;
    };
    bits_.erase(bit_it);
}


bool can::BitFrame::RemoveBit(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();

    if (bits_.size() <= index)
        return false;

    std::advance(bit_it, index);
    bits_.erase(bit_it);

    return true;
}


void can::BitFrame::RemoveBit(size_t index, BitType bit_type)
{
    Bit *bit_to_remove = GetBitOf(index, bit_type);
    assert(bit_to_remove != NULL && "Can't remove bit which is NULL");

    RemoveBit(bit_to_remove);
}

bool can::BitFrame::RemoveBitsFrom(size_t index)
{
    if (bits_.size() <= index)
        return false;

    while (bits_.size() > index)
    {
        auto end_it = bits_.end();
        end_it--;
        bits_.erase(end_it);
    }

    return true;
}

void can::BitFrame::RemoveBitsFrom(size_t index, BitType bit_type)
{
    RemoveBitsFrom(GetBitIndex(GetBitOf(index, bit_type)));
}

bool can::BitFrame::InsertErrorFlag(size_t index, BitType error_flag_type)
{
    Bit *bit = GetBit(index);

    /* We should not insert Error frame oinstead of SOF right away as real DUT
     * will never start transmitting errro frame right from SOF! */
    assert(index > 0);

    assert(error_flag_type == BitType::ActiveErrorFlag ||
           error_flag_type == BitType::PassiveErrorFlag);

    if (bit == nullptr)
        return false;

    // Discard all bits from this bit further
    ClearFrameBits(index);

    // If bit frame is inserted on bit in Data bit rate, correct PH2 of
    // previous bit so that it already counts in Nominal bit-rate!
    Bit *prev_bit = GetBit(index - 1);
    prev_bit->CorrectPh2LenToNominal();

    // Insert Error flag of according value
    BitValue value;
    if (error_flag_type == BitType::ActiveErrorFlag)
        value = BitValue::Dominant;
    else
        value = BitValue::Recessive;

    for (int i = 0; i < 6; i++)
        AppendBit(error_flag_type, value);

    return true;
}


bool can::BitFrame::InsertActiveErrorFrame(size_t index)
{
    if (InsertErrorFlag(index, BitType::ActiveErrorFlag) == false)
        return false;

    for (int i = 0; i < 8; i++)
        AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);

    // Insert intermission
    for (int i = 0; i < 3; i++)
        AppendBit(BitType::Intermission, BitValue::Recessive);

    return true;
}

bool can::BitFrame::InsertActiveErrorFrame(size_t index, BitType bit_type)
{
    return InsertActiveErrorFrame(GetBitOf(index, bit_type));
}

bool can::BitFrame::InsertActiveErrorFrame(Bit *bit)
{
    return InsertActiveErrorFrame(GetBitIndex(bit));
}


bool can::BitFrame::InsertPassiveErrorFrame(size_t index)
{
    if (InsertErrorFlag(index, BitType::PassiveErrorFlag) == false)
        return false;

    for (int i = 0; i < 8; i++)
        AppendBit(BitType::ErrorDelimiter, BitValue::Recessive);

    for (int i = 0; i < 3; i++)
        AppendBit(BitType::Intermission, BitValue::Recessive);

    return true;
}


bool can::BitFrame::InsertPassiveErrorFrame(Bit *bit)
{
    return InsertPassiveErrorFrame(GetBitIndex(bit));
}


bool can::BitFrame::InsertPassiveErrorFrame(size_t index, BitType bit_type)
{
    return InsertPassiveErrorFrame(GetBitOf(index, bit_type));
}


bool can::BitFrame::InsertOverloadFrame(size_t index)
{
    Bit *bit = GetBit(index);

    if (bit == nullptr)
        return false;

    if (bit->bit_type_ != BitType::Intermission &&
        bit->bit_type_ != BitType::ErrorDelimiter &&
        bit->bit_type_ != BitType::OverloadDelimiter)
    {
        std::cerr << " Can't insert Overload frame to this bit!" << std::endl;
        return false;
    }

    ClearFrameBits(index);

    for (int i = 0; i < 6; i++)
        AppendBit(BitType::OverloadFlag, BitValue::Dominant);
    for (int i = 0; i < 8; i++)
        AppendBit(BitType::OverloadDelimiter, BitValue::Recessive);

    for (int i = 0; i < 3; i++)
        AppendBit(BitType::Intermission, BitValue::Recessive);

    return true;
}


bool can::BitFrame::InsertOverloadFrame(Bit *bit)
{
    return InsertOverloadFrame(GetBitIndex(bit));
}


bool can::BitFrame::InsertOverloadFrame(size_t index, BitType bit_type)
{
    return InsertOverloadFrame(GetBitOf(index, bit_type));
}


void can::BitFrame::AppendSuspendTransmission()
{
    for (int i = 0; i < 8; i++)
        AppendBit(Bit(this, BitType::Suspend, BitValue::Recessive,
            &frame_flags_, nominal_bit_timing_, data_bit_timing_));
}


bool can::BitFrame::LooseArbitration(size_t index)
{
    Bit *bit = GetBit(index);
    std::list<Bit>::iterator bit_it = bits_.begin();

    if (bit == nullptr)
        return false;

    BitType bit_type = bit->bit_type_;
    if (bit_type != BitType::BaseIdentifier &&
        bit_type != BitType::IdentifierExtension &&
        bit_type != BitType::Rtr &&
        bit_type != BitType::Srr &&
        bit_type != BitType::Ide &&

        /* R1 is not in arbitration, but this is needed for simpler modelling in cases
           when arbitration is lost on RTR bit against FD frames (which have R1 bit there) */
        bit_type != BitType::R1)
    {
        std::cerr << "Can't loose arbitration on bit which is not in arbitration field" << std::endl;
        return false;
    }

    // Move to position where we want to loose arbitration
    // TODO: Rewrite with lambda!
    for (size_t i = 0; i < index; i++)
        bit_it++;

    /* Turn to recessive from this bit further */
    for (; bit_it != bits_.end(); bit_it++)
        bit_it->bit_value_ = BitValue::Recessive;

    GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;

    return true;
}


bool can::BitFrame::LooseArbitration(Bit *bit)
{
    return LooseArbitration(GetBitIndex(bit));
}


bool can::BitFrame::LooseArbitration(size_t index, BitType bit_type)
{
    return LooseArbitration(GetBitOf(index, bit_type));
}


void can::BitFrame::TurnReceivedFrame()
{
    for (auto bit_it = bits_.begin(); bit_it != bits_.end(); bit_it++)
        bit_it->bit_value_ = BitValue::Recessive;

    Bit *bit = GetBitOf(0, BitType::Ack);
    assert(bit != NULL && "No ACK bit present in frame!");
    bit->bit_value_ = BitValue::Dominant;
}


int can::BitFrame::GetNumStuffBits(BitType bit_type, StuffBitType stuff_bit_type)
{
    return std::count_if(bits_.begin(), bits_.end(), [bit_type, stuff_bit_type](Bit bit) {
        if (bit.bit_type_ == bit_type && bit.stuff_bit_type == stuff_bit_type)
            return true;
        return false;
    });
}


int can::BitFrame::GetNumStuffBits(BitType bit_type, StuffBitType stuff_bit_type,
                                   BitValue bit_value)
{
    return std::count_if(bits_.begin(), bits_.end(), [bit_type, stuff_bit_type, bit_value](Bit bit) {
        if (bit.bit_type_ == bit_type &&
            bit.stuff_bit_type == stuff_bit_type &&
            bit.bit_value_ == bit_value)
            return true;
        return false;
    });
}


int can::BitFrame::GetNumStuffBits(StuffBitType stuff_bit_type)
{
    return std::count_if(bits_.begin(), bits_.end(), [stuff_bit_type](Bit bit) {
        if (bit.stuff_bit_type == stuff_bit_type)
            return true;
        return false;
    });
}


int can::BitFrame::GetNumStuffBits(StuffBitType stuff_bit_type, BitValue bit_value)
{
    return std::count_if(bits_.begin(), bits_.end(), [stuff_bit_type, bit_value](Bit bit) {
        if (bit.stuff_bit_type == stuff_bit_type &&
            bit.bit_value_ == bit_value)
            return true;
        return false;
    });
}


void can::BitFrame::Print(bool print_stuff_bits)
{
    std::list<Bit>::iterator bit_it;

    std::string vals = "";
    std::string names = "";

    for (bit_it = bits_.begin(); bit_it != bits_.end();)
    {
        // Print separators betwen different field types (also prints separator
        //  at start of frame)
        vals += "|";
        names += " ";

        // Both methods advance iterator when bit is printed.
        if (bit_it->IsSingleBitField()) {
            //if (printStuffBits == false && bit->stuffBitType != NoStuffBit)
            //    continue;
            PrintSingleBitField(bit_it, &vals, &names, print_stuff_bits);
        } else {
            PrintMultiBitField(bit_it, &vals, &names, print_stuff_bits);
        }
    }

    std::cout << names << std::endl;
    std::cout << std::string(names.length(), '-') << std::endl;
    std::cout << vals << std::endl;
    std::cout << std::string(names.length(), '-') << std::endl;
}


void can::BitFrame::UpdateFrame(bool recalc_crc)
{
    // First remove all stuff bits!
    for (auto bit_it = bits_.begin(); bit_it != bits_.end(); bit_it++)
        if (bit_it->stuff_bit_type == StuffBitType::FixedStuffBit ||
            bit_it->stuff_bit_type == StuffBitType::NormalStuffBit)
            bit_it = bits_.erase(bit_it);

    // Recalculate CRC and add stuff bits!
    if (frame_flags().is_fdf_ == FrameType::Can2_0){
        if (recalc_crc) {
            CalculateCrc();
            UpdateCrcBits();
        }

        // We must set CRC before Stuff bits are inserted because in CAN 2.0
        // frames regular stuff bits are inserted also to CRC!
        InsertNormalStuffBits();
    } else {
        InsertNormalStuffBits();
        SetStuffCount();
        SetStuffParity();
        InsertStuffCountStuffBits();
        if (recalc_crc) {
            CalculateCrc();
            UpdateCrcBits();
        }
        InsertCrcFixedStuffBits();
    }
}


can::CycleBitValue* can::BitFrame::MoveCyclesBack(CycleBitValue *from, size_t move_by)
{
    std::list<can::Bit>::iterator curr_bit;
    std::list<can::TimeQuanta>::iterator curr_time_quanta;
    std::list<can::CycleBitValue>::iterator curr_cycle;

    /* Search for the TQ and bit which contains the cycle */
    //std::cout << "STARTING SEARCH" << std::endl;
    int bit_index = 0;
    for (curr_bit = bits_.begin(); curr_bit != bits_.end(); curr_bit++)
    {
        size_t tq_index = 0;
        for (curr_time_quanta = curr_bit->GetTimeQuantaIterator(0);; curr_time_quanta++)
        {
            size_t cycle_index = 0;
            for (curr_cycle = curr_time_quanta->GetCycleBitValueIterator(0);; curr_cycle++)
            {
                if (&(*curr_cycle) == from)
                    goto found;

                if (cycle_index == (curr_time_quanta->getLengthCycles() - 1))
                    break;
                cycle_index++;
            }
            if (tq_index == (curr_bit->GetLengthTimeQuanta() - 1))
                break;
            tq_index++;
        }
        bit_index++;
    }
    assert("Input cycle should be part of frame" && false);

found:
    /* Iterate back for required amount of cycles */
    size_t cnt = 0;
    do {
        if (curr_cycle == curr_time_quanta->GetCycleBitValueIterator(0)) {
            if (curr_time_quanta == curr_bit->GetTimeQuantaIterator(0)) {
                if (curr_bit == bits_.begin()) {
                    assert("Hit start of frame! Cant move so far!" && false);
                } else {
                    curr_bit--;
                    curr_time_quanta = curr_bit->GetLastTimeQuantaIterator(BitPhase::Ph2);
                    curr_cycle = curr_time_quanta->GetCycleBitValueIterator(
                                curr_time_quanta->getLengthCycles() - 1);
                }
            } else {
                curr_time_quanta--;
                curr_cycle = curr_time_quanta->GetCycleBitValueIterator(
                                curr_time_quanta->getLengthCycles() - 1);
            }
        } else {
            curr_cycle--;
        }
        cnt++;
    } while (cnt != move_by);

    return &(*curr_cycle);
}


void can::BitFrame::CompensateEdgeForInputDelay(Bit *from, int input_delay)
{
    [[maybe_unused]] Bit *prev_bit = GetBit(GetBitIndex(from) - 1);

    assert(from->bit_value_ == BitValue::Dominant &&
           "Input delay compensation shall end at Dominant bit");    
    assert(prev_bit->bit_value_ == BitValue::Recessive &&
           "Input delay compensation shall start at Recessive bit");

    CycleBitValue *cycle = from->GetFirstTimeQuantaIterator(BitPhase::Sync)->getCycleBitValue(0);
    for (int i = 0; i < input_delay; i++)
    {
        CycleBitValue *compensated_cycle = MoveCyclesBack(cycle, i + 1);
        compensated_cycle->ForceValue(BitValue::Dominant);
    }
}


void can::BitFrame::FlipBitAndCompensate(Bit *bit, int input_delay)
{
    bit->FlipBitValue();
    if (bit->bit_value_ == BitValue::Dominant)
        CompensateEdgeForInputDelay(bit, input_delay);
}


void can::BitFrame::PutAcknowledge()
{
    GetBitOf(0, BitType::Ack)->bit_value_ = BitValue::Dominant;
}


void can::BitFrame::PutAcknowledge(int input_delay)
{
    Bit *ack = GetBitOf(0, BitType::Ack);
    ack->bit_value_ = BitValue::Dominant;
    CompensateEdgeForInputDelay(ack, input_delay);
}


void can::BitFrame::PrintSingleBitField(std::list<Bit>::iterator& bit_it,
                                        std::string *vals,
                                        std::string *names,
                                        bool printStuffBits)
{
    std::list<Bit>::iterator nxtBitIt;
    nxtBitIt = bit_it;
    nxtBitIt++;

    // Print the bit itself
    vals->append(" " + bit_it->GetColouredValue() + " ");
    names->append(bit_it->GetBitTypeName());
    bit_it++;

    // Handle stuff bit. If stuff bit is inserted behind a single bit
    // field it is marked with the same bit field!
    if (nxtBitIt->bit_type_ == bit_it->bit_type_ &&
        (nxtBitIt->stuff_bit_type == StuffBitType::FixedStuffBit ||
         nxtBitIt->stuff_bit_type == StuffBitType::NormalStuffBit))
    {
        if (printStuffBits == true)
        {
            names->append(std::string(3, ' '));
            vals->append(" " + bit_it->GetColouredValue() + " ");
        }
        bit_it++;
    }
}

void can::BitFrame::PrintMultiBitField(std::list<Bit>::iterator& bit_it,
                                       std::string *vals,
                                       std::string *names,
                                       bool printStuffBits)
{
    int len = 0;
    int preOffset = 0;
    int postOffset = 0;
    std::string fieldName = bit_it->GetBitTypeName();
    std::list<Bit>::iterator firstBitIt = bit_it;

    for (; bit_it->bit_type_ == firstBitIt->bit_type_; bit_it++)
    {
        if (printStuffBits == false && bit_it->stuff_bit_type != StuffBitType::NoStuffBit)
            continue;

        len += 2;
        vals->append(bit_it->GetColouredValue() + " ");
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