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


can::Bit::Bit(BitFrame *parent, BitType bit_type, BitValue bit_value, FrameFlags* frame_flags,
              BitTiming* nominal_bit_timing, BitTiming* data_bit_timing):
              bit_type_(bit_type),
              bit_value_(bit_value),
              stuff_bit_type_(StuffBitType::NoStuffBit),
              parent_(parent),
              frame_flags_(frame_flags),
              nominal_bit_timing_(nominal_bit_timing),
              data_bit_timing_(data_bit_timing)
{
    ConstructTimeQuantas();
}


can::Bit::Bit(BitFrame *parent, BitType bit_type, BitValue bit_value, FrameFlags* frame_flags,
              BitTiming* nominal_bit_timing, BitTiming* data_bit_timing,
              StuffBitType stuff_bit_type) :
              bit_type_(bit_type),
              bit_value_(bit_value),
              stuff_bit_type_(stuff_bit_type),
              parent_(parent),
              frame_flags_(frame_flags),
              nominal_bit_timing_(nominal_bit_timing),
              data_bit_timing_(data_bit_timing)
{
    ConstructTimeQuantas();
}

const can::BitPhase can::Bit::default_bit_phases[] =
    {BitPhase::Sync, BitPhase::Prop, BitPhase::Ph1, BitPhase::Ph2};


void can::Bit::FlipBitValue()
{
    bit_value_ = GetOppositeValue();
}


can::BitValue can::Bit::GetOppositeValue()
{
    if (bit_value_ == BitValue::Dominant)
        return BitValue::Recessive;
    return BitValue::Dominant;
}


bool can::Bit::IsStuffBit()
{
    if (stuff_bit_type_ == StuffBitType::NormalStuffBit ||
        stuff_bit_type_ == StuffBitType::FixedStuffBit)
    {
        return true;
    }
    return false;
}


std::string can::Bit::GetBitTypeName()
{
    for (size_t i = 0; i < sizeof(bit_type_names_) / sizeof(BitTypeName); i++)
        if (bit_type_names_[i].bit_type == bit_type_)
            return bit_type_names_[i].name;
    return "<INVALID_BIT_TYPE_NAME>";
}


std::string can::Bit::GetColouredValue()
{
    // Put stuff bits green
    if (IsStuffBit())
        return "\033[1;32m" + std::to_string((int)bit_value_) + "\033[0m";

    // Error frame bits red
    else if (bit_type_ == BitType::ActiveErrorFlag ||
             bit_type_ == BitType::PassiveErrorFlag ||
             bit_type_ == BitType::ErrorDelimiter)
        return "\033[1;31m" + std::to_string((int)bit_value_) + "\033[0m";

    // Overload frame light blue
    else if (bit_type_ == BitType::OverloadFlag ||
             bit_type_ == BitType::OverloadDelimiter)
        return "\033[1;36m" + std::to_string((int)bit_value_) + "\033[0m";

    // Default color for other bit types
    else
        return std::to_string((int)bit_value_);
}


bool can::Bit::IsSingleBitField()
{
    if (bit_type_ == BitType::Sof ||
        bit_type_ == BitType::R0 ||
        bit_type_ == BitType::R1 ||
        bit_type_ == BitType::Srr ||
        bit_type_ == BitType::Rtr ||
        bit_type_ == BitType::Ide ||
        bit_type_ == BitType::Edl ||
        bit_type_ == BitType::Brs ||
        bit_type_ == BitType::Esi ||
        bit_type_ == BitType::CrcDelimiter ||
        bit_type_ == BitType::StuffParity ||
        bit_type_ == BitType::Ack ||
        bit_type_ == BitType::AckDelimiter)
        return true;

    return false;
}


bool can::Bit::HasPhase(BitPhase bit_phase)
{
    for (auto &time_quanta : time_quantas_)
        if (time_quanta.bit_phase() == bit_phase)
            return true;

    return false;
}


bool can::Bit::HasNonDefaultValues()
{
    for (auto &time_quanta : time_quantas_)
        if (time_quanta.HasNonDefaultValues())
            return true;

    return false;
}


size_t can::Bit::GetPhaseLenTimeQuanta(BitPhase bit_phase)
{
    return std::count_if(time_quantas_.begin(), time_quantas_.end(),
            [bit_phase](TimeQuanta time_quanta)
        {
            if (time_quanta.bit_phase() == bit_phase)
                return true;
            return false;
        });
}


size_t can::Bit::GetPhaseLenCycles(BitPhase bit_phase)
{
    int num_cycles = 0;

    for (auto &time_quanta : time_quantas_)
        if (time_quanta.bit_phase() == bit_phase)
            num_cycles += time_quanta.getLengthCycles();

    return num_cycles;
}


size_t can::Bit::GetLengthTimeQuanta()
{
    size_t num_time_quanta = 0;

    for (auto &phase : default_bit_phases)
        num_time_quanta += GetPhaseLenTimeQuanta(phase);

    return num_time_quanta;
}


size_t can::Bit::GetLengthCycles()
{
    size_t num_cycles = 0;

    for (auto &phase : default_bit_phases)
        num_cycles += GetPhaseLenCycles(phase);

    return num_cycles;
}


size_t can::Bit::ShortenPhase(BitPhase bit_phase, size_t num_time_quanta)
{
    size_t phase_len = GetPhaseLenTimeQuanta(bit_phase);
    size_t shorten_by = num_time_quanta;

    //std::cout << "Phase lenght: " << std::to_string(phase_len) << std::endl;
    //std::cout << "Shorten by: " << std::to_string(shorten_by) << std::endl;

    if (phase_len == 0)
        return 0;
    if (phase_len < num_time_quanta)
        shorten_by = phase_len;

    // Following assumes that phase is contiguous within a bit (resonable assumption)
    auto tq_it = GetLastTimeQuantaIterator(bit_phase);
    for (size_t i = 0; i < shorten_by; i++)
    {
        tq_it = time_quantas_.erase(tq_it);
        tq_it--;
    }

    return shorten_by;
}


void can::Bit::LengthenPhase(BitPhase bit_phase, size_t num_time_quanta)
{
    auto time_quanta_iterator = GetLastTimeQuantaIterator(bit_phase);

    /* 
     * GetLastTimeQuantaIterator returns Last time Quanta of phase or
     * first of next phase if phase does not exist. If it exist, move
     * to one further so that we append to the end
     */
    if (GetPhaseLenTimeQuanta(bit_phase) > 0)
        time_quanta_iterator++;

    BitTiming *bit_timing = GetPhaseBitTiming(bit_phase);
    for (size_t i = 0; i < num_time_quanta; i++)
        time_quantas_.insert(time_quanta_iterator, TimeQuanta(this, bit_timing->brp_, bit_phase));
}


std::list<can::TimeQuanta>::iterator can::Bit::GetTimeQuantaIterator(size_t index)
{
    assert(index < time_quantas_.size() && "Bit does not have so many time quantas");

    auto time_quanta_iterator = time_quantas_.begin();
    std::advance(time_quanta_iterator, index);
    return time_quanta_iterator;
}


can::TimeQuanta* can::Bit::GetTimeQuanta(size_t index)
{
    return &(*GetTimeQuantaIterator(index));
}

can::CycleBitValue* can::Bit::GetCycle(size_t index)
{
    auto tq = GetTimeQuantaIterator(0);
    auto cycle = tq->GetCycleBitValueIterator(0);
    size_t cnt = 0;
    while (cnt != index) {
        if (&(*cycle) == tq->getCycleBitValue(tq->getLengthCycles() - 1)) {
            tq++;
            cycle = tq->GetCycleBitValueIterator(0);
        } else {
            cycle++;
        }
        cnt++;
    }
    return &(*cycle);
}

size_t can::Bit::GetCycleIndex(can::CycleBitValue* cycle)
{
    auto tq = GetTimeQuantaIterator(0);
    auto curr_cycle = tq->GetCycleBitValueIterator(0);
    size_t cnt = 0;
    while (&(*curr_cycle) != cycle) {
        if (&(*curr_cycle) == tq->getCycleBitValue(tq->getLengthCycles() - 1)) {
            tq++;
            assert (tq != time_quantas_.end());
            curr_cycle = tq->GetCycleBitValueIterator(0);
        } else {
            curr_cycle++;
        }
        cnt++;
    }
    return cnt;
}

can::TimeQuanta* can::Bit::GetTimeQuanta(BitPhase bit_phase, size_t index)
{
    [[maybe_unused]] size_t phase_len = GetPhaseLenTimeQuanta(bit_phase);

    assert(phase_len > 0 && "Bit phase does not exist");
    assert(index < phase_len && "Bit does not have so many time quantas");

    // Assumes phase is contiguous, reasonable assumption.
    auto time_quanta_iterator = GetFirstTimeQuantaIterator(bit_phase);
    std::advance(time_quanta_iterator, index);

    return &(*time_quanta_iterator);
}


bool can::Bit::ForceTimeQuanta(size_t index, BitValue bit_value)
{
    if (index >= GetLengthTimeQuanta())
        return false;

    auto time_quanta_iterator = time_quantas_.begin();
    std::advance(time_quanta_iterator, index);
    time_quanta_iterator->ForceValue(bit_value);

    return true;
}


size_t can::Bit::ForceTimeQuanta(size_t start_index, size_t end_index, BitValue bit_value)
{
    size_t len_time_quanta = GetLengthTimeQuanta();

    if (start_index >= len_time_quanta || start_index > end_index)
        return 0;

    size_t end_index_clamp = end_index;
    if (end_index >= len_time_quanta)
        end_index_clamp = len_time_quanta - 1;

    auto time_quanta_iterator = time_quantas_.begin();
    std::advance(time_quanta_iterator, start_index);

    size_t i = 0;
    for (; i <= end_index_clamp - start_index; i++)
    {
        time_quanta_iterator->ForceValue(bit_value);
        time_quanta_iterator++;
    }

    return i;
}


bool can::Bit::ForceTimeQuanta(size_t index, BitPhase bit_phase, BitValue bit_value)
{
    size_t phase_len = GetPhaseLenTimeQuanta(bit_phase);

    if ((phase_len == 0) || (phase_len <= index))
        return false;

    GetTimeQuanta(bit_phase, index)->ForceValue(bit_value);

    return true;
}


size_t can::Bit::ForceTimeQuanta(size_t start_index, size_t end_index, BitPhase bit_phase,
                                 BitValue bit_value)
{
    size_t phase_len = GetPhaseLenTimeQuanta(bit_phase);
    if ((phase_len == 0) || (phase_len <= start_index))
        return false;

    size_t end_index_clamp = end_index;
    if (end_index >= phase_len)
        end_index_clamp = phase_len - 1;

    size_t i;
    for (i = start_index; i <= end_index_clamp; i++)
        ForceTimeQuanta(i, bit_phase, bit_value);

    return i;
}


can::BitPhase can::Bit::PrevBitPhase(BitPhase bit_phase)
{
    switch (bit_phase)
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

can::BitPhase can::Bit::NextBitPhase(BitPhase bit_phase)
{
    switch (bit_phase)
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

can::BitRate can::Bit::GetPhaseBitRate(BitPhase bit_phase)
{
    if (frame_flags_->is_fdf() == FrameType::CanFd && frame_flags_->is_brs() == BrsFlag::Shift)
    {
        switch (bit_type_) {
        case BitType::Brs:
            if (bit_phase == BitPhase::Ph2)
                return BitRate::Data;
            return BitRate::Nominal;

        case BitType::CrcDelimiter:
            if (bit_phase == BitPhase::Ph2)
                return BitRate::Nominal;
            return BitRate::Data;

        case BitType::Esi:
        case BitType::Dlc:
        case BitType::Data:
        case BitType::StuffCount:
        case BitType::StuffParity:
        case BitType::Crc:
            return BitRate::Data;

        default:
            break;
        }
    }
    return BitRate::Nominal;
}


can::BitTiming* can::Bit::GetPhaseBitTiming(BitPhase bit_phase)
{
    if (GetPhaseBitRate(bit_phase) == BitRate::Nominal)
        return nominal_bit_timing_;

    return data_bit_timing_;
}


void can::Bit::CorrectPh2LenToNominal()
{
    /* If bit Phase 2 is in data bit rate, then correct its lenght to nominal */

    if (GetPhaseBitTiming(BitPhase::Ph2) == data_bit_timing_)
    {
        std::cout << "Compensating PH2 of " << GetBitTypeName() <<
                     " bit due to inserted Error frame!" << std::endl;
        std::cout << "Lenght before compensation: " <<
                    std::to_string(GetLengthCycles()) << std::endl;

        // Remove all PH2 phases
        for (auto tqIter = time_quantas_.begin(); tqIter != time_quantas_.end();)
             if (tqIter->bit_phase() == BitPhase::Ph2)
                 tqIter = time_quantas_.erase(tqIter);
             else
                 tqIter++;

        // Re-create again with nominal bit timing
        for (size_t i = 0; i < nominal_bit_timing_->ph2_; i++)
            time_quantas_.push_back(TimeQuanta(this, nominal_bit_timing_->brp_, BitPhase::Ph2));

        std::cout << "Lenght after compensation: " <<
                    std::to_string(GetLengthCycles()) << std::endl;            
    }

}


std::list<can::TimeQuanta>::iterator can::Bit::GetFirstTimeQuantaIterator(BitPhase bit_phase)
{
    if (HasPhase(bit_phase))
    {
        return std::find_if(time_quantas_.begin(), time_quantas_.end(), [bit_phase](TimeQuanta tq)
            {
                if (tq.bit_phase() == bit_phase)
                    return true;
                return false;
            });
    }
    return GetLastTimeQuantaIterator(PrevBitPhase(bit_phase));
}


std::list<can::TimeQuanta>::iterator
    can::Bit::GetLastTimeQuantaIterator(BitPhase bit_phase)
{
    if (HasPhase(bit_phase))
    {
        auto iterator = GetFirstTimeQuantaIterator(bit_phase);
        while (iterator->bit_phase() == bit_phase)
            iterator++;
        return --iterator;
    }
    return GetFirstTimeQuantaIterator(NextBitPhase(bit_phase));
}


void can::Bit::ConstructTimeQuantas()
{
    BitTiming *tseg1_bit_timing = nominal_bit_timing_;
    BitTiming *tseg2_bit_timing = nominal_bit_timing_;

    // Here Assume that PH1 has the same bit rate as TSEG1 which is reasonable
    // as there is no bit-rate shift within TSEG1
    if (GetPhaseBitRate(BitPhase::Ph1) == BitRate::Data)
        tseg1_bit_timing = data_bit_timing_;
    if (GetPhaseBitRate(BitPhase::Ph2) == BitRate::Data)
        tseg2_bit_timing = data_bit_timing_;

    // Construct TSEG 1
    time_quantas_.push_back(TimeQuanta(this, tseg1_bit_timing->brp_, BitPhase::Sync));
    for (size_t i = 0; i < tseg1_bit_timing->prop_; i++)
        time_quantas_.push_back(TimeQuanta(this, tseg1_bit_timing->brp_, BitPhase::Prop));
    for (size_t i = 0; i < tseg1_bit_timing->ph1_; i++)
        time_quantas_.push_back(TimeQuanta(this, tseg1_bit_timing->brp_, BitPhase::Ph1));

    // Construct TSEG 2
    for (size_t i = 0; i < tseg2_bit_timing->ph2_; i++)
        time_quantas_.push_back(TimeQuanta(this, tseg2_bit_timing->brp_, BitPhase::Ph2));
}