/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 *
 *****************************************************************************/

#include <iostream>
#include <assert.h>
#include <iomanip>

#include "can.h"
#include "Bit.h"
#include "BitFrame.h"

void can::BitFrame::ConstructFrame()
{
    BuildFrameBits();

    if (frame_flags().is_fdf() == FrameKind::Can20){
        CalcCrc();
        UpdateCrcBits();

        // We must set CRC before Stuff bits are inserted because in CAN 2.0
        // frames regular stuff bits are inserted also to CRC!
        InsertNormalStuffBits();
    } else {
        InsertNormalStuffBits();
        SetStuffCnt();
        SetStuffParity();
        InsertStuffToStuffCnt();
        CalcCrc();
        UpdateCrcBits();
        InsertFixedStuffToCrc();
    }
}


can::BitFrame::BitFrame(FrameFlags frame_flags, uint8_t dlc, int ident,
                        uint8_t *data, BitTiming* nbt, BitTiming* dbt):
                Frame(frame_flags, dlc, ident, data)
{
    nbt_ = nbt;
    dbt_ = dbt;

    ConstructFrame();
}


can::BitFrame::BitFrame(Frame &frame, BitTiming *nbt, BitTiming *dbt):
                Frame(frame.frame_flags(), frame.dlc(),
                      frame.identifier(), frame.data())
{
    nbt_ = nbt;
    dbt_ = dbt;

    ConstructFrame();
}


void can::BitFrame::UpdateCrcBits()
{
    std::list<Bit>::iterator bit_it = GetBitOfIter(0, BitKind::Crc);
    uint32_t tmp_crc = crc();
    int i;

    if (frm_flags_.is_fdf() == FrameKind::Can20)
        i = 14;
    else if (data_len_ > 16)
        i = 20;
    else
        i = 16;

    while (bit_it->kind_ == BitKind::Crc)
    {
        // CRC should be set in CAN FD frames before stuff bits in CRC are
        // inserted (as CRC affects value of these stuff bits), therefore
        // it is illegal to calculate CRC when stuff bits in it are already
        // inserted!
        assert(bit_it->stuff_kind_ == StuffKind::NoStuff);

        bit_it->val_ = (BitVal)((tmp_crc >> i) & 0x1);
        i--;
        bit_it++;
    }
}


uint32_t can::BitFrame::base_ident()
{
    if (frm_flags_.is_ide() == IdentKind::Ext)
        return ((uint32_t)(identifier() >> 18));
    else
        return (uint32_t)identifier();
}


uint32_t can::BitFrame::ext_ident()
{
    if (frm_flags_.is_ide() == IdentKind::Ext)
        return (uint32_t)(identifier()) & 0x3FFFF;
    else
        return 0;
}


uint8_t can::BitFrame::stuff_count()
{
    if (frm_flags_.is_fdf() == FrameKind::Can20){
        std::cerr << "CAN 2.0 frame does not have Stuff count field defined" << std::endl;
        return 0;
    }
    return stuff_cnt_;
}


uint32_t can::BitFrame::crc()
{
    if (frm_flags_.is_fdf() == FrameKind::Can20)
        return crc15_;
    if (data_len_ > 16)
        return crc21_;
    return crc17_;
}


// LSB represents bit value we want to append
void can::BitFrame::AppendBit(BitKind kind, int value)
{
    BitVal bit_value;

    if ((value % 2) == 0)
        bit_value = BitVal::Dominant;
    else
        bit_value = BitVal::Recessive;

    AppendBit(kind, bit_value);
}


void can::BitFrame::AppendBit(BitKind kind, BitVal value)
{
    bits_.push_back(Bit(this, kind, value, &frm_flags_, nbt_, dbt_));
}


void can::BitFrame::AppendBitFrame(can::BitFrame *bit_frame)
{
    for (size_t i = 0; i < bit_frame->GetLen(); i++)
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
    AppendBit(BitKind::Sof, BitVal::Dominant);

    // Build base ID
    uint32_t base_id = base_ident();
    for (int i = 10; i >= 0; i--)
        AppendBit(BitKind::BaseIdent, base_id >> i);

    // Build RTR/r1/SRR
    if (frm_flags_.is_ide() == IdentKind::Ext) {
        AppendBit(BitKind::Srr, BitVal::Recessive);
    } else {
        if (frm_flags_.is_fdf() == FrameKind::CanFd) {
            AppendBit(BitKind::R1, BitVal::Dominant);
        } else {
            if (frm_flags_.is_rtr() == RtrFlag::Rtr)
                AppendBit(BitKind::Rtr, BitVal::Recessive);
            else
                AppendBit(BitKind::Rtr, BitVal::Dominant);
        }
    }

    // Build IDE, Extended Identifier and one bit post Extended Identifier
    if (frm_flags_.is_ide() == IdentKind::Ext) {
        AppendBit(BitKind::Ide, BitVal::Recessive);

        uint32_t extId = ext_ident();
        for (int i = 17; i >= 0; i--)
            AppendBit(BitKind::ExtIdent, extId >> i);

        if (frm_flags_.is_fdf() == FrameKind::CanFd) {
            AppendBit(BitKind::R1, BitVal::Dominant);
        } else {
            if (frm_flags_.is_rtr() == RtrFlag::Rtr) {
                AppendBit(BitKind::Rtr, BitVal::Recessive);
            } else {
                AppendBit(BitKind::Rtr, BitVal::Dominant);
            }
        }
    } else {
        AppendBit(BitKind::Ide, BitVal::Dominant);
    }

    // Build EDL/r0/r1 bit
    if (frm_flags_.is_fdf() == FrameKind::CanFd) {
        AppendBit(BitKind::Edl, BitVal::Recessive);
    } else if (frm_flags_.is_ide() == IdentKind::Ext) {
        AppendBit(BitKind::R1, BitVal::Dominant);
    } else {
        AppendBit(BitKind::R0, BitVal::Dominant);
    }

    // Build extra r0 past EDL or in Extended Identifier frame
    if (frm_flags_.is_fdf() == FrameKind::CanFd || frm_flags_.is_ide() == IdentKind::Ext) {
        AppendBit(BitKind::R0, BitVal::Dominant);
    }

    // Build BRS and ESI bits
    if (frm_flags_.is_fdf() == FrameKind::CanFd) {
        if (frm_flags_.is_brs() == BrsFlag::DoShift)
            AppendBit(BitKind::Brs, BitVal::Recessive);
        else
            AppendBit(BitKind::Brs, BitVal::Dominant);

        if (frm_flags_.is_esi() == EsiFlag::ErrAct)
            AppendBit(BitKind::Esi, BitVal::Dominant);
        else
            AppendBit(BitKind::Esi, BitVal::Recessive);
    }

    // Build DLC
    for (int i = 3; i >= 0; i--)
        AppendBit(BitKind::Dlc, dlc_ >> i);

    // Build data field
    for (int i = 0; i < data_length(); i++) {
        for (int j = 7; j >= 0; j--)
            AppendBit(BitKind::Data, data(i) >> j);
    }

    // Build Stuff count + parity (put dummy as we don't know number of
    // stuff bits yet)!
    if (frm_flags_.is_fdf() == FrameKind::CanFd)
    {
        for (int i = 0; i < 3; i++)
            AppendBit(BitKind::StuffCnt, BitVal::Dominant);
        AppendBit(BitKind::StuffParity, BitVal::Recessive);
    }

    // Build CRC - put dummies so far since we don't have Stuff bits
    // yet, we can't calculate value of CRC for CAN FD frames!
    int crc_length;

    if (frm_flags_.is_fdf() == FrameKind::Can20)
        crc_length = 15;
    else if (data_length() <= 16)
        crc_length = 17;
    else
        crc_length = 21;

    for (int i = crc_length - 1; i >= 0; i--)
        AppendBit(BitKind::Crc, BitVal::Recessive);

    // Add CRC Delimiter, ACK and ACK Delimiter
    AppendBit(BitKind::CrcDelim, BitVal::Recessive);
    AppendBit(BitKind::Ack, BitVal::Recessive);
    if (frm_flags_.is_fdf() == FrameKind::CanFd)
        AppendBit(BitKind::Ack, BitVal::Recessive);
    AppendBit(BitKind::AckDelim, BitVal::Recessive);

    // Finalize by EOF and by Intermission
    for (int i = 0; i < 7; i++)
        AppendBit(BitKind::Eof, BitVal::Recessive);
    for (int i = 0; i < 3; i++)
        AppendBit(BitKind::Interm, BitVal::Recessive);
}


size_t can::BitFrame::InsertNormalStuffBits()
{
    std::list<Bit>::iterator bit_it;
    int same_bits = 1;
    stuff_cnt_ = 0;
    BitVal prev_value = BitVal::Dominant; // As if SOF

    if (bits_.front().kind_ != BitKind::Sof) {
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
        if (bit_it->kind_ == BitKind::CrcDelim ||
            bit_it->kind_ == BitKind::StuffCnt ||
            bit_it == bits_.end())
            break;

        if (bit_it->val_ == prev_value)
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
            if (bit_it_nxt->kind_ == BitKind::StuffCnt)
            {
                prev_value = bit_it->val_;
                continue;
            }

            Bit bit = Bit(this, bit_it->kind_, bit_it->GetOppositeVal(),
                          &frm_flags_, nbt_, dbt_,
                          StuffKind::Normal);
            bit_it++;
            bit_it = bits_.insert(bit_it, bit);
            same_bits = 1;

            stuff_cnt_ = static_cast<uint8_t>((stuff_cnt_ + 1) % 8);
        }
        prev_value = bit_it->val_;
    }

    return stuff_cnt_;
}


void can::BitFrame::InsertStuffToStuffCnt()
{
    std::list<Bit>::iterator bit_it;
    BitVal stuff_bit_value;

    assert(!(frm_flags_.is_fdf() == FrameKind::Can20));

    for (bit_it = bits_.begin(); bit_it->kind_ != BitKind::StuffCnt; bit_it++)
        ;
    bit_it--;
    stuff_bit_value = bit_it->GetOppositeVal();
    bit_it++;

    Bit bit = Bit(this, BitKind::StuffCnt, stuff_bit_value, &frm_flags_,
                  nbt_, dbt_, StuffKind::Fixed);
    bit_it = bits_.insert(bit_it, bit);
    bit_it++;

    // Move one beyond stuff parity and calculate stuff bit post parity
    for (int i = 0; i < 3; i++)
        bit_it++;
    stuff_bit_value = bit_it->GetOppositeVal();
    bit_it->GetOppositeVal();
    bit_it++;

    Bit bit_2 = Bit(this, BitKind::StuffParity, stuff_bit_value, &frm_flags_,
                    nbt_, dbt_, StuffKind::Fixed);
    bit_it = bits_.insert(bit_it, bit_2);
}


void can::BitFrame::InsertFixedStuffToCrc()
{
    std::list<Bit>::iterator bit_it;
    int same_bits = 0;

    // Search first bit of CRC
    for (bit_it = bits_.begin(); bit_it->kind_ != BitKind::Crc; bit_it++)
        ;

    for (; bit_it->kind_ != BitKind::CrcDelim; ++bit_it)
    {
        same_bits++;
        if ((same_bits % 4) == 0)
        {
            Bit bit = Bit(this, BitKind::Crc, bit_it->GetOppositeVal(),
                          &frm_flags_, nbt_, dbt_,
                          StuffKind::Fixed);
            bit_it = bits_.insert(++bit_it, bit);
        }
    }

}


uint32_t can::BitFrame::CalcCrc()
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
        if (bit_it->kind_ == BitKind::Crc)
            break;

        BitVal bit_value = bit_it->val_;
        crc_nxt_15 = (uint32_t)(bit_value) ^ ((crc15_ >> 14) & 0x1);
        crc_nxt_17 = (uint32_t)(bit_value) ^ ((crc17_ >> 16) & 0x1);
        crc_nxt_21 = (uint32_t)(bit_value) ^ ((crc21_ >> 20) & 0x1);

        // Shift left, CRC 15 always without stuff bits
        if (bit_it->stuff_kind_ == StuffKind::NoStuff)
            crc15_ = (crc15_ << 1);

        if (bit_it->stuff_kind_ != StuffKind::Fixed)
        {
            crc17_ = (crc17_ << 1);
            crc21_ = (crc21_ << 1);
        }

        crc15_ &= 0x7FFF;
        crc17_ &= 0x1FFFF;
        crc21_ &= 0x1FFFFF;

        // Calculate by polynomial
        if ((crc_nxt_15 == 1) && (bit_it->stuff_kind_ == StuffKind::NoStuff))
            crc15_ ^= 0xC599;
        if ((crc_nxt_17 == 1) && (bit_it->stuff_kind_ != StuffKind::Fixed))
            crc17_ ^= 0x3685B;
        if ((crc_nxt_21 == 1) && (bit_it->stuff_kind_ != StuffKind::Fixed))
            crc21_ ^= 0x302899;

        bit_it++;
    }

    //printf("Calculated CRC 15 : 0x%x\n", crc15_);
    //printf("Calculated CRC 17 : 0x%x\n", crc17_);
    //printf("Calculated CRC 21 : 0x%x\n", crc21_);

    if (frm_flags_.is_fdf() == FrameKind::Can20)
        return crc15_;
    else if (data_len_ <= 16)
        return crc17_;
    else
        return crc21_;
}


bool can::BitFrame::SetStuffCnt()
{
    std::list<Bit>::iterator bit_it;
    stuff_cnt_encoded_ = 0;
    bit_it = bits_.begin();

    // DontShift sense to try to set Stuff count on CAN 2.0 frames!
    if (frm_flags_.is_fdf() == FrameKind::Can20)
        return false;

    while (bit_it->kind_ != BitKind::StuffCnt && bit_it != bits_.end())
        bit_it++;

    if (bit_it == bits_.end())
    {
        std::cerr << "Did not find stuff count field!" << std::endl;
        return false;
    }

    assert(stuff_cnt_ < 8);

    switch (stuff_cnt_){
        case 0x0 :
            stuff_cnt_encoded_ = 0b000;
            break;
        case 0x1:
            stuff_cnt_encoded_ = 0b001;
            break;
        case 0x2 :
            stuff_cnt_encoded_ = 0b011;
            break;
        case 0x3 :
            stuff_cnt_encoded_ = 0b010;
            break;
        case 0x4 :
            stuff_cnt_encoded_ = 0b110;
            break;
        case 0x5 :
            stuff_cnt_encoded_ = 0b111;
            break;
        case 0x6 :
            stuff_cnt_encoded_ = 0b101;
            break;
        case 0x7 :
            stuff_cnt_encoded_ = 0b100;
            break;
    };

    for (int i = 2; i >= 0; i--)
    {
        assert(bit_it->kind_ == BitKind::StuffCnt);
        bit_it->val_ = (BitVal)((stuff_cnt_encoded_ >> i) & 0x1);
        bit_it++;
    }
    return true;
}


bool can::BitFrame::SetStuffParity()
{
    std::list<Bit>::iterator bit_it;
    uint8_t val = 0;

    if (frm_flags_.is_fdf() == FrameKind::Can20)
        return false;

    for (bit_it = bits_.begin(); bit_it->kind_ != BitKind::StuffParity; bit_it++)
        ;
    for (int i = 0; i < 3; i++)
        val ^= (stuff_cnt_encoded_ >> i) & 0x1;
    bit_it->val_ = (BitVal)val;

    return true;
}


size_t can::BitFrame::GetLen()
{
    return bits_.size();
}


size_t can::BitFrame::GetFieldLen(BitKind bit_type)
{
    return std::count_if(bits_.begin(), bits_.end(),
                         [bit_type](can::Bit bit) { return bit.kind_ == bit_type; });
}


can::Bit* can::BitFrame::GetRandBitOf(BitKind bit_type)
{
    size_t bit_field_length = GetFieldLen(bit_type);
    assert(bit_field_length > 0 && "Frame has no bits of required type!");

    std::list<Bit>::iterator bit_it =
        std::find_if(bits_.begin(), bits_.end(),
                     [bit_type](Bit bit) { return bit.kind_ == bit_type; });

    size_t bit_index = rand() % bit_field_length;
    std::advance(bit_it, bit_index);

    return &(*bit_it);
}

can::Bit* can::BitFrame::GetRandBit(BitVal bit_value)
{
    Bit *bit;
    size_t lenght = this->GetLen();
    do
    {
        bit = GetBit(rand() % lenght);
    } while (bit->val_ != bit_value);

    return bit;
}

can::Bit* can::BitFrame::GetBit(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();

    assert(bits_.size() > index && "Insufficient number of bits in a frame!");

    std::advance(bit_it, index);
    return &(*bit_it);
}

std::list<can::Bit>::iterator can::BitFrame::GetBitIter(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();

    assert(bits_.size() > index && "Insufficient number of bits in a frame!");

    std::advance(bit_it, index);
    return bit_it;
}


can::Bit* can::BitFrame::GetBitOf(size_t index, BitKind bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (bit_it != bits_.end())
    {
        if (bit_it->kind_ == bit_type)
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


can::Bit* can::BitFrame::GetBitOfNoStuffBits(size_t index, BitKind bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (bit_it != bits_.end())
    {
        if (bit_it->kind_ == bit_type &&
            bit_it->stuff_kind_ == StuffKind::NoStuff)
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


std::list<can::Bit>::iterator can::BitFrame::GetBitOfIter(size_t index, BitKind bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (bit_it != bits_.end())
    {
        if (bit_it->kind_ == bit_type)
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


can::Bit* can::BitFrame::GetStuffBit(size_t index)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if (bit_it->stuff_kind_ == StuffKind::Normal ||
            bit_it->stuff_kind_ == StuffKind::Fixed)
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}

can::Bit* can::BitFrame::GetStuffBit(size_t index, BitKind bit_type)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if ((bit_it->stuff_kind_ == StuffKind::Normal ||
             bit_it->stuff_kind_ == StuffKind::Fixed) &&
            (bit_it->kind_ == bit_type))
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}

can::Bit* can::BitFrame::GetStuffBit(BitKind bit_type, StuffKind stuff_bit_type,
                                     BitVal bit_value)
{
    auto bit_it = std::find_if(bits_.begin(), bits_.end(),
                               [bit_type, stuff_bit_type, bit_value] (Bit bit) {
        if (bit.kind_ == bit_type &&
            bit.val_ == bit_value &&
            bit.stuff_kind_ == stuff_bit_type)
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
        if (bit_it->stuff_kind_ == StuffKind::Fixed)
            i++;
    }

    if (bit_it == bits_.end())
        return nullptr;

    return &(*bit_it);
}


can::Bit* can::BitFrame::GetFixedStuffBit(size_t index, BitVal bit_value)
{
    std::list<Bit>::iterator bit_it = bits_.begin();
    size_t i = 0;

    while (i <= index && bit_it != bits_.end())
    {
        bit_it++;
        if (bit_it->stuff_kind_ == StuffKind::Fixed &&
            bit_it->val_ == bit_value)
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


bool can::BitFrame::InsertBit(BitKind bit_type, BitVal bit_value, size_t index)
{
    return InsertBit(Bit(this, bit_type, bit_value, &frm_flags_, nbt_,
                         dbt_), index);
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


void can::BitFrame::RemoveBit(size_t index, BitKind bit_type)
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

void can::BitFrame::RemoveBitsFrom(size_t index, BitKind bit_type)
{
    RemoveBitsFrom(GetBitIndex(GetBitOf(index, bit_type)));
}

bool can::BitFrame::InsertErrFlag(size_t index, BitKind error_flag_type)
{
    Bit *bit = GetBit(index);

    /* We should not insert Error frame oinstead of SOF right away as real DUT
     * will never start transmitting error frame right from SOF! */
    assert(index > 0);

    assert(error_flag_type == BitKind::ActErrFlag ||
           error_flag_type == BitKind::PasErrFlag);

    if (bit == nullptr)
        return false;

    // Discard all bits from this bit further
    ClearFrameBits(index);

    // If bit frame is inserted on bit in Data bit rate, correct PH2 of
    // previous bit so that it already counts in Nominal bit-rate!
    Bit *prev_bit = GetBit(index - 1);
    prev_bit->CorrectPh2LenToNominal();

    // Insert Error flag of according value
    BitVal value;
    if (error_flag_type == BitKind::ActErrFlag)
        value = BitVal::Dominant;
    else
        value = BitVal::Recessive;

    for (int i = 0; i < 6; i++)
        AppendBit(error_flag_type, value);

    return true;
}


bool can::BitFrame::InsertActErrFrm(size_t index)
{
    if (InsertErrFlag(index, BitKind::ActErrFlag) == false)
        return false;

    for (int i = 0; i < 8; i++)
        AppendBit(BitKind::ErrDelim, BitVal::Recessive);

    // Insert intermission
    for (int i = 0; i < 3; i++)
        AppendBit(BitKind::Interm, BitVal::Recessive);

    return true;
}

bool can::BitFrame::InsertActErrFrm(size_t index, BitKind bit_type)
{
    return InsertActErrFrm(GetBitOf(index, bit_type));
}

bool can::BitFrame::InsertActErrFrm(Bit *bit)
{
    return InsertActErrFrm(GetBitIndex(bit));
}


bool can::BitFrame::InsertPasErrFrm(size_t index)
{
    if (InsertErrFlag(index, BitKind::PasErrFlag) == false)
        return false;

    for (int i = 0; i < 8; i++)
        AppendBit(BitKind::ErrDelim, BitVal::Recessive);

    for (int i = 0; i < 3; i++)
        AppendBit(BitKind::Interm, BitVal::Recessive);

    return true;
}


bool can::BitFrame::InsertPasErrFrm(Bit *bit)
{
    return InsertPasErrFrm(GetBitIndex(bit));
}


bool can::BitFrame::InsertPasErrFrm(size_t index, BitKind bit_type)
{
    return InsertPasErrFrm(GetBitOf(index, bit_type));
}


bool can::BitFrame::InsertOvrlFrm(size_t index)
{
    Bit *bit = GetBit(index);

    if (bit == nullptr)
        return false;

    if (bit->kind_ != BitKind::Interm &&
        bit->kind_ != BitKind::ErrDelim &&
        bit->kind_ != BitKind::OvrlDelim)
    {
        std::cerr << " Can't insert Overload frame to this bit!" << std::endl;
        return false;
    }

    ClearFrameBits(index);

    for (int i = 0; i < 6; i++)
        AppendBit(BitKind::OvrlFlag, BitVal::Dominant);
    for (int i = 0; i < 8; i++)
        AppendBit(BitKind::OvrlDelim, BitVal::Recessive);

    for (int i = 0; i < 3; i++)
        AppendBit(BitKind::Interm, BitVal::Recessive);

    return true;
}


bool can::BitFrame::InsertOvrlFrm(Bit *bit)
{
    return InsertOvrlFrm(GetBitIndex(bit));
}


bool can::BitFrame::InsertOvrlFrm(size_t index, BitKind bit_type)
{
    return InsertOvrlFrm(GetBitOf(index, bit_type));
}


void can::BitFrame::AppendSuspTrans()
{
    for (int i = 0; i < 8; i++)
        AppendBit(Bit(this, BitKind::SuspTrans, BitVal::Recessive,
            &frm_flags_, nbt_, dbt_));
}


bool can::BitFrame::LooseArbit(size_t index)
{
    Bit *bit = GetBit(index);
    std::list<Bit>::iterator bit_it = bits_.begin();

    if (bit == nullptr)
        return false;

    BitKind bit_type = bit->kind_;
    if (bit_type != BitKind::BaseIdent &&
        bit_type != BitKind::ExtIdent &&
        bit_type != BitKind::Rtr &&
        bit_type != BitKind::Srr &&
        bit_type != BitKind::Ide &&

        /* R1 is not in arbitration, but this is needed for simpler modelling in cases
           when arbitration is lost on RTR bit against FD frames (which have R1 bit there) */
        bit_type != BitKind::R1)
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
        bit_it->val_ = BitVal::Recessive;

    GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;

    return true;
}


bool can::BitFrame::LooseArbit(Bit *bit)
{
    return LooseArbit(GetBitIndex(bit));
}


bool can::BitFrame::LooseArbit(size_t index, BitKind bit_type)
{
    return LooseArbit(GetBitOf(index, bit_type));
}


void can::BitFrame::ConvRXFrame()
{
    for (auto bit_it = bits_.begin(); bit_it != bits_.end(); bit_it++)
        bit_it->val_ = BitVal::Recessive;

    Bit *bit = GetBitOf(0, BitKind::Ack);
    assert(bit != NULL && "No ACK bit present in frame!");
    bit->val_ = BitVal::Dominant;
}


size_t can::BitFrame::GetNumStuffBits(BitKind bit_type, StuffKind stuff_bit_type)
{
    return std::count_if(bits_.begin(), bits_.end(), [bit_type, stuff_bit_type](Bit bit) {
        if (bit.kind_ == bit_type && bit.stuff_kind_ == stuff_bit_type)
            return true;
        return false;
    });
}


size_t can::BitFrame::GetNumStuffBits(BitKind bit_type, StuffKind stuff_bit_type,
                                   BitVal bit_value)
{
    return std::count_if(bits_.begin(), bits_.end(), [bit_type, stuff_bit_type, bit_value](Bit bit) {
        if (bit.kind_ == bit_type &&
            bit.stuff_kind_ == stuff_bit_type &&
            bit.val_ == bit_value)
            return true;
        return false;
    });
}


size_t can::BitFrame::GetNumStuffBits(StuffKind stuff_bit_type)
{
    return std::count_if(bits_.begin(), bits_.end(), [stuff_bit_type](Bit bit) {
        if (bit.stuff_kind_ == stuff_bit_type)
            return true;
        return false;
    });
}


size_t can::BitFrame::GetNumStuffBits(StuffKind stuff_bit_type, BitVal bit_value)
{
    return std::count_if(bits_.begin(), bits_.end(), [stuff_bit_type, bit_value](Bit bit) {
        if (bit.stuff_kind_ == stuff_bit_type &&
            bit.val_ == bit_value)
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


void can::BitFrame::PrintDetailed(std::chrono::nanoseconds clock_period)
{
    std::cout
        << std::setw (20) << "Field"
        << std::setw (20) << "Duration (ns)"
        << std::setw (20) << "Value" << std::endl;

    for (auto & bit : bits_)
    {
        std::chrono::nanoseconds bit_length = bit.GetLenCycles() * clock_period;

        std::cout
            << std::setw (20) << bit.GetBitKindName()
            << std::setw (20) << std::to_string(bit_length.count())
            << std::setw (19) << " " << bit.GetColouredVal() << std::endl;
    }
}



void can::BitFrame::UpdateFrame(bool recalc_crc)
{
    // First remove all stuff bits!
    for (auto bit_it = bits_.begin(); bit_it != bits_.end(); bit_it++)
        if (bit_it->stuff_kind_ == StuffKind::Fixed ||
            bit_it->stuff_kind_ == StuffKind::Normal)
            bit_it = bits_.erase(bit_it);

    // Recalculate CRC and add stuff bits!
    if (frame_flags().is_fdf() == FrameKind::Can20){
        if (recalc_crc) {
            CalcCrc();
            UpdateCrcBits();
        }

        // We must set CRC before Stuff bits are inserted because in CAN 2.0
        // frames regular stuff bits are inserted also to CRC!
        InsertNormalStuffBits();
    } else {
        InsertNormalStuffBits();
        SetStuffCnt();
        SetStuffParity();
        InsertStuffToStuffCnt();
        if (recalc_crc) {
            CalcCrc();
            UpdateCrcBits();
        }
        InsertFixedStuffToCrc();
    }
}


can::Cycle* can::BitFrame::MoveCyclesBack(Cycle *from, size_t move_by)
{
    std::list<can::Bit>::iterator curr_bit;
    std::list<can::TimeQuanta>::iterator curr_time_quanta;
    std::list<can::Cycle>::iterator curr_cycle;

    /* Search for the TQ and bit which contains the cycle */
    //std::cout << "STARTING SEARCH" << std::endl;
    int bit_index = 0;
    curr_bit = bits_.begin();
    while (curr_bit != bits_.end())
    {
        size_t tq_index = 0;
        curr_time_quanta = curr_bit->GetTQIter(0);
        while (true)
        {
            size_t cycle_index = 0;
            curr_cycle = curr_time_quanta->GetCycleBitValIter(0);
            while (true)
            {
                if (&(*curr_cycle) == from)
                    goto found;

                if (cycle_index == (curr_time_quanta->getLengthCycles() - 1))
                    break;
                cycle_index++;
                curr_cycle++;
            }
            if (tq_index == (curr_bit->GetLenTQ() - 1))
                break;
            tq_index++;
            curr_time_quanta++;
        }
        bit_index++;
        curr_bit++;
    }
    assert("Input cycle should be part of frame" && false);

found:
    /* Iterate back for required amount of cycles */
    size_t cnt = 0;
    while (cnt < move_by) {
        if (curr_cycle == curr_time_quanta->GetCycleBitValIter(0)) {
            if (curr_time_quanta == curr_bit->GetTQIter(0)) {
                if (curr_bit == bits_.begin()) {
                    assert("Hit start of frame! Cant move so far!" && false);
                } else {
                    curr_bit--;
                    curr_time_quanta = curr_bit->GetTQIter(
                                        curr_bit->GetLenTQ() - 1);
                    curr_cycle = curr_time_quanta->GetCycleBitValIter(
                                    curr_time_quanta->getLengthCycles() - 1);
                }
            } else {
                curr_time_quanta--;
                curr_cycle = curr_time_quanta->GetCycleBitValIter(
                                curr_time_quanta->getLengthCycles() - 1);
            }
        } else {
            curr_cycle--;
        }
        cnt++;
    }

    return &(*curr_cycle);
}


void can::BitFrame::CompensateEdgeForInputDelay(Bit *from, int input_delay)
{
    [[maybe_unused]] Bit *prev_bit = GetBit(GetBitIndex(from) - 1);

    assert(from->val_ == BitVal::Dominant &&
           "Input delay compensation shall end at Dominant bit");
    assert(prev_bit->val_ == BitVal::Recessive &&
           "Input delay compensation shall start at Recessive bit");

    Cycle *cycle = from->GetTQ(0)->getCycleBitValue(0);
    for (int i = 0; i < input_delay; i++)
    {
        Cycle *compensated_cycle = MoveCyclesBack(cycle, i + 1);
        compensated_cycle->ForceVal(BitVal::Dominant);
    }
}


void can::BitFrame::FlipBitAndCompensate(Bit *bit, int input_delay)
{
    bit->FlipVal();

    size_t index = GetBitIndex(bit);

    // If we are flipping bit 0, then there will be no edge introduced!
    if (index == 0)
        return;

    Bit *prev_bit = GetBit(index - 1);

    // We need to compensate if flipped value is Dominant and Previous bit is Recessive.
    // If this condition is met, then we insert unwanted synchronization edge to IUTs
    // CAN RX stream. Therefore we need to adjust position of this edge accordingly not
    // to cause undesired resynchronization!
    if (bit->val_ == BitVal::Dominant && prev_bit->val_ == BitVal::Recessive)
        CompensateEdgeForInputDelay(bit, input_delay);
}


void can::BitFrame::PutAck()
{
    GetBitOf(0, BitKind::Ack)->val_ = BitVal::Dominant;
}


void can::BitFrame::PutAck(int input_delay)
{
    Bit *ack = GetBitOf(0, BitKind::Ack);
    ack->val_ = BitVal::Dominant;
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
    vals->append(" " + bit_it->GetColouredVal() + " ");
    names->append(bit_it->GetBitKindName());
    bit_it++;

    // Handle stuff bit. If stuff bit is inserted behind a single bit
    // field it is marked with the same bit field!
    if (nxtBitIt->kind_ == bit_it->kind_ &&
        (nxtBitIt->stuff_kind_ == StuffKind::Fixed ||
         nxtBitIt->stuff_kind_ == StuffKind::Normal))
    {
        if (printStuffBits == true)
        {
            names->append(std::string(3, ' '));
            vals->append(" " + bit_it->GetColouredVal() + " ");
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
    std::string fieldName = bit_it->GetBitKindName();
    std::list<Bit>::iterator firstBitIt = bit_it;

    for (; bit_it->kind_ == firstBitIt->kind_; bit_it++)
    {
        if (printStuffBits == false && bit_it->stuff_kind_ != StuffKind::NoStuff)
            continue;

        len += 2;
        vals->append(bit_it->GetColouredVal() + " ");
    }

    preOffset = (len - static_cast<int>(fieldName.length())) / 2;
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