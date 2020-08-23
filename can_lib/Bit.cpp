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


can::Bit::Bit(BitType bit_type, BitValue bit_value, FrameFlags* frame_flags,
              BitTiming* nominal_bit_timing, BitTiming* data_bit_timing)
{
    this->bit_type_ = bit_type;
    this->bit_value_ = bit_value;
    this->stuff_bit_type = StuffBitType::NoStuffBit;

    this->frame_flags = frame_flags;
    this->nominal_bit_timing = nominal_bit_timing;
    this->data_bit_timing = data_bit_timing;

    ConstructTimeQuantas();
}


can::Bit::Bit(BitType bit_type, BitValue bit_value, FrameFlags* frame_flags,
              BitTiming* nominal_bit_timing, BitTiming* data_bit_timing,
              StuffBitType stuff_bit_type)
{
    this->bit_type_ = bit_type;
    this->bit_value_ = bit_value;
    this->stuff_bit_type = stuff_bit_type;
    
    this->frame_flags = frame_flags;
    this->nominal_bit_timing = nominal_bit_timing;
    this->data_bit_timing = data_bit_timing;

    ConstructTimeQuantas();
}


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
    if (stuff_bit_type == StuffBitType::NormalStuffBit ||
        stuff_bit_type == StuffBitType::FixedStuffBit)
    {
        return true;
    }
    return false;
}


std::string can::Bit::GetBitTypeName()
{
    for (int i = 0; i < sizeof(bit_type_names_) / sizeof(BitTypeName); i++)
        if (bit_type_names_[i].bit_type == bit_type_)
            return bit_type_names_[i].name;
    return " ";
}


std::string can::Bit::GetColouredValue()
{
    if (IsStuffBit())
        return "\033[1;32m" + std::to_string((int)bit_value_) + "\033[0m";
    else if (bit_type_ == BitType::ActiveErrorFlag ||
             bit_type_ == BitType::PassiveErrorFlag ||
             bit_type_ == BitType::ErrorDelimiter)
        return "\033[1;31m" + std::to_string((int)bit_value_) + "\033[0m";
    else if (bit_type_ == BitType::OverloadFlag ||
             bit_type_ == BitType::OverloadDelimiter)
        return "\033[1;36m" + std::to_string((int)bit_value_) + "\033[0m";
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
    for (auto time_quanta : time_quantas_)
        if (time_quanta.bit_phase == bit_phase)
            return true;

    return false;
}


bool can::Bit::HasNonDefaultValues()
{
    for (auto time_quanta : time_quantas_)
        if (time_quanta.HasNonDefaultValues())
            return true;

    return false;
}


void can::Bit::SetAllDefaultValues()
{
    for (auto time_quanta : time_quantas_)
        time_quanta.SetAllDefaultValues();
}


int can::Bit::GetPhaseLenTimeQuanta(BitPhase bit_phase)
{
    int num_time_quanta = 0;

    for (auto time_quanta : time_quantas_)
        if (time_quanta.bit_phase == bit_phase)
            num_time_quanta++;

    return num_time_quanta;
}


int can::Bit::GetPhaseLenCycles(BitPhase bit_phase)
{
    int num_cycles = 0;

    for (auto time_quanta : time_quantas_)
        if (time_quanta.bit_phase == bit_phase)
            num_cycles += time_quanta.getLengthCycles();

    return num_cycles;
}


int can::Bit::GetLengthTimeQuanta()
{
    int num_time_quanta = 0;

    num_time_quanta += GetPhaseLenTimeQuanta(BitPhase::Sync);
    num_time_quanta += GetPhaseLenTimeQuanta(BitPhase::Prop);
    num_time_quanta += GetPhaseLenTimeQuanta(BitPhase::Ph1);
    num_time_quanta += GetPhaseLenTimeQuanta(BitPhase::Ph2);

    return num_time_quanta;
}


int can::Bit::GetLengthCycles()
{
    int num_cycles = 0;

    num_cycles += GetPhaseLenCycles(BitPhase::Sync);
    num_cycles += GetPhaseLenCycles(BitPhase::Prop);
    num_cycles += GetPhaseLenCycles(BitPhase::Ph1);
    num_cycles += GetPhaseLenCycles(BitPhase::Ph2);

    return num_cycles;
}


int can::Bit::ShortenPhase(BitPhase bit_phase, int num_time_quanta)
{
    int phase_len = GetPhaseLenTimeQuanta(bit_phase);
    int shorten_by = num_time_quanta;

    std::cout << "Phase lenght: " << std::to_string(phase_len) << std::endl;
    std::cout << "Shorten by: " << std::to_string(shorten_by) << std::endl;

    if (phase_len == 0)
        return 0;
    if (phase_len < num_time_quanta)
        shorten_by = phase_len;

    auto time_quanta_iterator = GetLastTimeQuantaIterator(bit_phase);
    for (int i = 0; i < shorten_by; i++){
        time_quanta_iterator = time_quantas_.erase(time_quanta_iterator);
        time_quanta_iterator--;
    }

    return shorten_by;
}


void can::Bit::LengthenPhase(BitPhase bit_phase, int num_time_quanta)
{
    int phase_len = GetPhaseLenTimeQuanta(bit_phase);
    auto time_quanta_iterator = GetLastTimeQuantaIterator(bit_phase);

    /* 
     * GetLastTimeQuantaIterator returns Last time Quanta of phase or
     * first of next phase if phase does not exist. If it exist, move
     * to one further so that we append to the end
     */
    if (phase_len > 0)
        time_quanta_iterator++;

    BitTiming *bit_timing = GetPhaseBitTiming(bit_phase);

    for (int i = 0; i < num_time_quanta; i++)
        time_quantas_.insert(time_quanta_iterator,
                             TimeQuanta(bit_timing->brp_, bit_phase));
}


can::TimeQuanta* can::Bit::GetTimeQuanta(int index)
{
    assert(("Bit does not have so many time quantas", index < time_quantas_.size()));

    auto time_quanta_iterator = time_quantas_.begin();
    std::advance(time_quanta_iterator, index);

    return &(*time_quanta_iterator);
}


can::TimeQuanta* can::Bit::GetTimeQuanta(BitPhase bit_phase, int index)
{
    int phase_len = GetPhaseLenTimeQuanta(bit_phase);

    assert(("Bit phase does not exist", phase_len > 0));
    assert(("Bit does not have so many time quantas", index < phase_len));

    auto time_quanta_iterator = GetFirstTimeQuantaIterator(bit_phase);
    std::advance(time_quanta_iterator, index);

    return &(*time_quanta_iterator);
}


bool can::Bit::ForceTimeQuanta(int index, BitValue bit_value)
{
    if (index >= GetLengthTimeQuanta())
        return false;

    auto time_quanta_iterator = time_quantas_.begin();
    std::advance(time_quanta_iterator, index);
    time_quanta_iterator->ForceValue(bit_value);

    return true;
}


int can::Bit::ForceTimeQuanta(int start_index, int end_index, BitValue bit_value)
{
    int len_time_quanta = GetLengthTimeQuanta();

    if (start_index >= len_time_quanta)
        return 0;
    if (start_index > end_index)
        return 0;

    int end_index_real = end_index;
    if (end_index >= len_time_quanta)
        end_index_real = len_time_quanta - 1;

    auto time_quanta_iterator = time_quantas_.begin();
    std::advance(time_quanta_iterator, start_index);

    int i = 0;
    for (; i <= end_index_real - start_index; i++)
    {
        time_quanta_iterator->ForceValue(bit_value);
        time_quanta_iterator++;
    }

    return i;
}


bool can::Bit::ForceTimeQuanta(int index, BitPhase bit_phase, BitValue bit_value)
{
    int phase_len = GetPhaseLenTimeQuanta(bit_phase);

    if ((phase_len == 0) || (phase_len <= index))
        return false;

    TimeQuanta *time_quanta = GetTimeQuanta(bit_phase, index);
    time_quanta->ForceValue(bit_value);

    return true;
}


int can::Bit::ForceTimeQuanta(int start_index, int end_index, BitPhase bit_phase,
                              BitValue bit_value)
{
    int phase_len = GetPhaseLenTimeQuanta(bit_phase);
    if ((phase_len == 0) || (phase_len <= start_index))
        return false;

    int end_index_real = end_index;
    if (end_index >= phase_len)
        end_index_real = phase_len - 1;

    int i;
    for (i = start_index; i <= end_index_real; i++)
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
        return BitPhase::Sync;
    }
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
        return BitPhase::Ph2;
    }
}


can::BitRate can::Bit::GetPhaseBitRate(BitPhase bit_phase)
{
    if (frame_flags->is_fdf_ == FrameType::CanFd && frame_flags->is_brs_ == BrsFlag::Shift)
    {
        switch (bit_type_) {
        case BitType::Brs:
            if (bit_phase == BitPhase::Ph2)
                return BitRate::Data;
            else
                return BitRate::Nominal;

        case BitType::CrcDelimiter:
            if (bit_phase == BitPhase::Ph2)
                return BitRate::Nominal;
            else
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
    BitRate bit_rate = GetPhaseBitRate(bit_phase);

    if (bit_rate == BitRate::Nominal)
        return nominal_bit_timing;
    else
        return data_bit_timing;
}


void can::Bit::CorrectPh2LenToNominal()
{
    // If bit Phase 2 is in data bit rate, then correct it to nominal
    if (GetPhaseBitTiming(BitPhase::Ph2) == data_bit_timing)
    {
        for (auto tqIter = time_quantas_.begin(); tqIter != time_quantas_.end();)
            if (tqIter->bit_phase == BitPhase::Ph2)
                tqIter = time_quantas_.erase(tqIter);
            else
                tqIter++;

        for (int i = 0; i < nominal_bit_timing->ph2_; i++)
            time_quantas_.push_back(TimeQuanta(nominal_bit_timing->brp_, BitPhase::Ph2));
    }
}


std::list<can::TimeQuanta>::iterator
can::Bit::GetFirstTimeQuantaIterator(BitPhase bit_phase)
{
    if (HasPhase(bit_phase))
    {
        auto iterator = time_quantas_.begin();
        while (iterator->bit_phase != bit_phase)
            iterator++;
        return iterator;
    }
    return GetLastTimeQuantaIterator(PrevBitPhase(bit_phase));
}


std::list<can::TimeQuanta>::iterator
    can::Bit::GetLastTimeQuantaIterator(BitPhase bit_phase)
{
    if (HasPhase(bit_phase))
    {
        auto iterator = GetFirstTimeQuantaIterator(bit_phase);
        while (iterator->bit_phase == bit_phase)
            iterator++;
        return --iterator;
    }
    return GetFirstTimeQuantaIterator(NextBitPhase(bit_phase));
}


void can::Bit::ConstructTimeQuantas()
{
    BitTiming *tseg1_bit_timing = this->nominal_bit_timing;
    BitTiming *tseg2_bit_timing = this->nominal_bit_timing;

    // Here Assume that PH1 has the same bit rate as TSEG1 which is reasonable
    // as there is no bit-rate shift within TSEG1
    BitRate tseg1_bit_rate = GetPhaseBitRate(BitPhase::Ph1);
    BitRate tseg2_bit_rate = GetPhaseBitRate(BitPhase::Ph2);

    if (tseg1_bit_rate == BitRate::Data)
        tseg1_bit_timing = data_bit_timing;
    if (tseg2_bit_rate == BitRate::Data)
        tseg2_bit_timing = data_bit_timing;

    // Construct TSEG 1
    time_quantas_.push_back(TimeQuanta(tseg1_bit_timing->brp_, BitPhase::Sync));
    for (int i = 0; i < tseg1_bit_timing->prop_; i++)
        time_quantas_.push_back(TimeQuanta(tseg1_bit_timing->brp_, BitPhase::Prop));
    for (int i = 0; i < tseg1_bit_timing->ph1_; i++)
        time_quantas_.push_back(TimeQuanta(tseg1_bit_timing->brp_, BitPhase::Ph1));

    // Construct TSEG 2
    for (int i = 0; i < tseg2_bit_timing->ph2_; i++)
        time_quantas_.push_back(TimeQuanta(tseg2_bit_timing->brp_, BitPhase::Ph2));
}