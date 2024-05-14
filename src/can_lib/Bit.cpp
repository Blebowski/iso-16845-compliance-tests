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

#include "can.h"
#include "BitTiming.h"
#include "TimeQuanta.h"
#include "FrameFlags.h"
#include "Bit.h"


can::Bit::Bit(BitFrame *parent, BitKind kind, BitVal val, FrameFlags* frm_flags,
              BitTiming* nbt, BitTiming* dbt):
              kind_(kind),
              val_(val),
              stuff_kind_(StuffKind::NoStuff),
              parent_(parent),
              frm_flags_(frm_flags),
              nbt_(nbt),
              dbt_(dbt)
{
    ConstructTQs();
}


can::Bit::Bit(BitFrame *parent, BitKind kind, BitVal val, FrameFlags* frm_flags,
              BitTiming* nbt, BitTiming* dbt, StuffKind stuff_kind) :
              kind_(kind),
              val_(val),
              stuff_kind_(stuff_kind),
              parent_(parent),
              frm_flags_(frm_flags),
              nbt_(nbt),
              dbt_(dbt)
{
    ConstructTQs();
}

const can::BitPhase can::Bit::def_bit_phases[] =
    {BitPhase::Sync, BitPhase::Prop, BitPhase::Ph1, BitPhase::Ph2};


void can::Bit::FlipVal()
{
    val_ = GetOppositeVal();
}


can::BitVal can::Bit::GetOppositeVal()
{
    if (val_ == BitVal::Dominant)
        return BitVal::Recessive;
    return BitVal::Dominant;
}


bool can::Bit::IsStuffBit()
{
    if (stuff_kind_ == StuffKind::Normal ||
        stuff_kind_ == StuffKind::Fixed)
    {
        return true;
    }
    return false;
}


std::string can::Bit::GetBitKindName()
{
    for (size_t i = 0; i < sizeof(bit_kind_names_) / sizeof(BitKindName); i++)
        if (bit_kind_names_[i].kind == kind_)
            return bit_kind_names_[i].name;
    return "<INVALID_BIT_TYPE_NAME>";
}


std::string can::Bit::GetColouredVal()
{
    // Put stuff bits green
    if (IsStuffBit())
        return "\033[1;32m" + std::to_string((int)val_) + "\033[0m";

    // Error frame bits red
    else if (kind_ == BitKind::ActErrFlag ||
             kind_ == BitKind::PasErrFlag ||
             kind_ == BitKind::ErrDelim)
        return "\033[1;31m" + std::to_string((int)val_) + "\033[0m";

    // Overload frame light blue
    else if (kind_ == BitKind::OvrlFlag ||
             kind_ == BitKind::OvrlDelim)
        return "\033[1;36m" + std::to_string((int)val_) + "\033[0m";

    // Default color for other bit types
    else
        return std::to_string((int)val_);
}


bool can::Bit::IsSingleBitField()
{
    if (kind_ == BitKind::Sof ||
        kind_ == BitKind::R0 ||
        kind_ == BitKind::R1 ||
        kind_ == BitKind::Srr ||
        kind_ == BitKind::Rtr ||
        kind_ == BitKind::Ide ||
        kind_ == BitKind::Edl ||
        kind_ == BitKind::Brs ||
        kind_ == BitKind::Esi ||
        kind_ == BitKind::CrcDelim ||
        kind_ == BitKind::StuffParity ||
        kind_ == BitKind::Ack ||
        kind_ == BitKind::AckDelim)
        return true;

    return false;
}


bool can::Bit::HasPhase(BitPhase phase)
{
    for (auto &tq : tqs_)
        if (tq.bit_phase() == phase)
            return true;

    return false;
}


bool can::Bit::HasNonDefVals()
{
    for (auto &tq : tqs_)
        if (tq.HasNonDefVals())
            return true;

    return false;
}


size_t can::Bit::GetPhaseLenTQ(BitPhase phase)
{
    return std::count_if(tqs_.begin(), tqs_.end(),
            [phase](TimeQuanta tq)
        {
            if (tq.bit_phase() == phase)
                return true;
            return false;
        });
}


size_t can::Bit::GetPhaseLenCycles(BitPhase phase)
{
    size_t num_cycles = 0;

    for (auto &tq : tqs_)
        if (tq.bit_phase() == phase)
            num_cycles += tq.getLengthCycles();

    return num_cycles;
}


size_t can::Bit::GetLenTQ()
{
    size_t n_tqs = 0;

    for (auto &phase : def_bit_phases)
        n_tqs += GetPhaseLenTQ(phase);

    return n_tqs;
}


size_t can::Bit::GetLenCycles()
{
    size_t n_cycles = 0;

    for (auto &phase : def_bit_phases)
        n_cycles += GetPhaseLenCycles(phase);

    return n_cycles;
}


size_t can::Bit::ShortenPhase(BitPhase phase, size_t n_tqs)
{
    size_t phase_len = GetPhaseLenTQ(phase);
    size_t shorten_by = n_tqs;

    //std::cout << "Phase lenght: " << std::to_string(phase_len) << std::endl;
    //std::cout << "Shorten by: " << std::to_string(shorten_by) << std::endl;

    if (phase_len == 0)
        return 0;
    if (phase_len < n_tqs)
        shorten_by = phase_len;

    // Following assumes that phase is contiguous within a bit (resonable assumption)
    auto tq_it = GetLastTQIter(phase);
    for (size_t i = 0; i < shorten_by; i++)
    {
        tq_it = tqs_.erase(tq_it);
        tq_it--;
    }

    return shorten_by;
}


void can::Bit::LengthenPhase(BitPhase phase, size_t n_tqs)
{
    auto tq_iter = GetLastTQIter(phase);

    /*
     * GetLastTimeQuantaIterator returns Last time Quanta of phase or
     * first of next phase if phase does not exist. If it exist, move
     * to one further so that we append to the end
     */
    if (GetPhaseLenTQ(phase) > 0)
        tq_iter++;

    BitTiming *timing = GetPhaseBitTiming(phase);
    for (size_t i = 0; i < n_tqs; i++)
        tqs_.insert(tq_iter, TimeQuanta(this, timing->brp_, phase));
}


std::list<can::TimeQuanta>::iterator can::Bit::GetTQIter(size_t index)
{
    assert(index < tqs_.size() && "Bit does not have so many time quantas");

    auto tq_iter = tqs_.begin();
    std::advance(tq_iter, index);
    return tq_iter;
}


can::TimeQuanta* can::Bit::GetTQ(size_t index)
{
    return &(*GetTQIter(index));
}

can::Cycle* can::Bit::GetCycle(size_t index)
{
    auto tq = GetTQIter(0);
    auto cycle = tq->GetCycleBitValIter(0);
    size_t cnt = 0;
    while (cnt != index) {
        if (&(*cycle) == tq->getCycleBitValue(tq->getLengthCycles() - 1)) {
            tq++;
            cycle = tq->GetCycleBitValIter(0);
        } else {
            cycle++;
        }
        cnt++;
    }
    return &(*cycle);
}

size_t can::Bit::GetCycleIndex(can::Cycle* cycle)
{
    auto tq = GetTQIter(0);
    auto curr_cycle = tq->GetCycleBitValIter(0);
    size_t cnt = 0;
    while (&(*curr_cycle) != cycle) {
        if (&(*curr_cycle) == tq->getCycleBitValue(tq->getLengthCycles() - 1)) {
            tq++;
            assert (tq != tqs_.end());
            curr_cycle = tq->GetCycleBitValIter(0);
        } else {
            curr_cycle++;
        }
        cnt++;
    }
    return cnt;
}

can::TimeQuanta* can::Bit::GetTQ(BitPhase phase, size_t index)
{
    [[maybe_unused]] size_t phase_len = GetPhaseLenTQ(phase);

    assert(phase_len > 0 && "Bit phase does not exist");
    assert(index < phase_len && "Bit does not have so many time quantas");

    // Assumes phase is contiguous, reasonable assumption.
    auto time_quanta_iterator = GetFirstTQIter(phase);
    std::advance(time_quanta_iterator, index);

    return &(*time_quanta_iterator);
}


bool can::Bit::ForceTQ(size_t index, BitVal value)
{
    if (index >= GetLenTQ())
        return false;

    auto tq_iter = tqs_.begin();
    std::advance(tq_iter, index);
    tq_iter->ForceVal(value);

    return true;
}


size_t can::Bit::ForceTQ(size_t start, size_t end, BitVal value)
{
    size_t len_tq = GetLenTQ();

    if (start >= len_tq || start > end)
        return 0;

    size_t end_index_clamp = end;
    if (end >= len_tq)
        end_index_clamp = len_tq - 1;

    auto tq_it = tqs_.begin();
    std::advance(tq_it, start);

    size_t i = 0;
    for (; i <= end_index_clamp - start; i++) {
        tq_it->ForceVal(value);
        tq_it++;
    }

    return i;
}


bool can::Bit::ForceTQ(size_t index, BitPhase phase, BitVal value)
{
    size_t phase_len = GetPhaseLenTQ(phase);

    if ((phase_len == 0) || (phase_len <= index))
        return false;

    GetTQ(phase, index)->ForceVal(value);

    return true;
}


size_t can::Bit::ForceTQ(size_t start, size_t end, BitPhase phase, BitVal value)
{
    size_t phase_len = GetPhaseLenTQ(phase);
    if ((phase_len == 0) || (phase_len <= start))
        return false;

    size_t end_index_clamp = end;
    if (end >= phase_len)
        end_index_clamp = phase_len - 1;

    size_t i;
    for (i = start; i <= end_index_clamp; i++)
        ForceTQ(i, phase, value);

    return i;
}


can::BitPhase can::Bit::PrevBitPhase(BitPhase phase)
{
    switch (phase)
    {
    case BitPhase::Ph2:
        if (HasPhase(BitPhase::Ph1))
            return BitPhase::Ph1;
        if (HasPhase(BitPhase::Prop))
            return BitPhase::Prop;

        // Assume here Sync phase is always there. We can't remove
        // it by Bit time settings. Having bit without sync phase
        // means corrupted bit!
        assert(HasPhase(BitPhase::Sync));
        return BitPhase::Sync;

    case BitPhase::Ph1:
        if (HasPhase(BitPhase::Prop))
            return BitPhase::Prop;

        assert(HasPhase(BitPhase::Sync));
        return BitPhase::Sync;

    case BitPhase::Prop:
        assert(HasPhase(BitPhase::Sync));
        return BitPhase::Sync;

    // In case of Sync phase do not link to previous bit in any way...
    case BitPhase::Sync:
        break;
    }

    return BitPhase::Sync;
}

can::BitPhase can::Bit::NextBitPhase(BitPhase phase)
{
    switch (phase)
    {
    case BitPhase::Sync:
        if (HasPhase(BitPhase::Prop))
            return BitPhase::Prop;
        if (HasPhase(BitPhase::Ph1))
            return BitPhase::Ph1;
        if (HasPhase(BitPhase::Ph2))
            return BitPhase::Ph2;
        return BitPhase::Sync;

    case BitPhase::Prop:
        if (HasPhase(BitPhase::Ph1))
            return BitPhase::Ph1;
        if (HasPhase(BitPhase::Ph2))
            return BitPhase::Ph2;
        return BitPhase::Prop;

    case BitPhase::Ph1:
        if (HasPhase(BitPhase::Ph2))
            return BitPhase::Ph2;
        return BitPhase::Ph1;

    case BitPhase::Ph2:
        break;
    }
    return BitPhase::Ph2;
}

can::BitRate can::Bit::GetPhaseBitRate(BitPhase phase)
{
    if (frm_flags_->is_fdf() == FrameKind::CanFd &&
        frm_flags_->is_brs() == BrsFlag::DoShift)
    {
        switch (kind_) {
        case BitKind::Brs:
            if (phase == BitPhase::Ph2)
                return BitRate::Data;
            return BitRate::Nominal;

        case BitKind::CrcDelim:
            if (phase == BitPhase::Ph2)
                return BitRate::Nominal;
            return BitRate::Data;

        case BitKind::Esi:
        case BitKind::Dlc:
        case BitKind::Data:
        case BitKind::StuffCnt:
        case BitKind::StuffParity:
        case BitKind::Crc:
            return BitRate::Data;

        default:
            break;
        }
    }
    return BitRate::Nominal;
}


can::BitTiming* can::Bit::GetPhaseBitTiming(BitPhase phase)
{
    if (GetPhaseBitRate(phase) == BitRate::Nominal)
        return nbt_;

    return dbt_;
}


void can::Bit::CorrectPh2LenToNominal()
{
    /* If bit Phase 2 is in data bit rate, then correct its lenght to nominal */

    if (GetPhaseBitTiming(BitPhase::Ph2) == dbt_)
    {
        std::cout << "Compensating PH2 of " << GetBitKindName() <<
                     " bit due to inserted Error frame!" << std::endl;
        std::cout << "Lenght before compensation: " <<
                    std::to_string(GetLenCycles()) << std::endl;

        // Remove all PH2 phases
        for (auto tqIter = tqs_.begin(); tqIter != tqs_.end();)
             if (tqIter->bit_phase() == BitPhase::Ph2)
                 tqIter = tqs_.erase(tqIter);
             else
                 tqIter++;

        // Re-create again with nominal bit timing
        for (size_t i = 0; i < nbt_->ph2_; i++)
            tqs_.push_back(TimeQuanta(this, nbt_->brp_, BitPhase::Ph2));

        std::cout << "Lenght after compensation: " <<
                    std::to_string(GetLenCycles()) << std::endl;
    }

}


std::list<can::TimeQuanta>::iterator can::Bit::GetFirstTQIter(BitPhase phase)
{
    if (HasPhase(phase))
    {
        return std::find_if(tqs_.begin(), tqs_.end(), [phase](TimeQuanta tq)
            {
                if (tq.bit_phase() == phase)
                    return true;
                return false;
            });
    }
    return GetLastTQIter(PrevBitPhase(phase));
}


std::list<can::TimeQuanta>::iterator can::Bit::GetLastTQIter(BitPhase phase)
{
    if (HasPhase(phase))
    {
        auto iterator = GetFirstTQIter(phase);
        while (iterator->bit_phase() == phase)
            iterator++;
        return --iterator;
    }
    return GetFirstTQIter(NextBitPhase(phase));
}


void can::Bit::ConstructTQs()
{
    BitTiming *tseg1_bt = nbt_;
    BitTiming *tseg2_bt = nbt_;

    // Here Assume that PH1 has the same bit rate as TSEG1 which is reasonable
    // as there is no bit-rate shift within TSEG1
    if (GetPhaseBitRate(BitPhase::Ph1) == BitRate::Data)
        tseg1_bt = dbt_;
    if (GetPhaseBitRate(BitPhase::Ph2) == BitRate::Data)
        tseg2_bt = dbt_;

    // Construct TSEG 1
    tqs_.push_back(TimeQuanta(this, tseg1_bt->brp_, BitPhase::Sync));
    for (size_t i = 0; i < tseg1_bt->prop_; i++)
        tqs_.push_back(TimeQuanta(this, tseg1_bt->brp_, BitPhase::Prop));
    for (size_t i = 0; i < tseg1_bt->ph1_; i++)
        tqs_.push_back(TimeQuanta(this, tseg1_bt->brp_, BitPhase::Ph1));

    // Construct TSEG 2
    for (size_t i = 0; i < tseg2_bt->ph2_; i++)
        tqs_.push_back(TimeQuanta(this, tseg2_bt->brp_, BitPhase::Ph2));
}