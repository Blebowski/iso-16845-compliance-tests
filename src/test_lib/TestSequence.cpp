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

#include "test_lib.h"
#include "../can_lib/can.h"

#include "TestSequence.h"
#include "../can_lib/BitFrame.h"

#include "../vpi_lib/vpiComplianceLib.hpp"

test_lib::TestSequence::TestSequence(std::chrono::nanoseconds clock_period)
{
    this->clock_period = clock_period;
}


test_lib::TestSequence::TestSequence(std::chrono::nanoseconds clock_period,
                                     can::BitFrame& frame,
                                     SequenceType sequence_type)
{
    this->clock_period = clock_period;

    if (sequence_type == SequenceType::DRIVER_SEQUENCE) {
        driven_values.clear();
        AppendDriverFrame(frame);
    } else {
        monitored_values.clear();
        AppendMonitorFrame(frame);
    }
}


test_lib::TestSequence::TestSequence(std::chrono::nanoseconds clock_period,
                                     can::BitFrame& driver_frame,
                                     can::BitFrame& monitor_frame)
{
    this->clock_period = clock_period;
    monitored_values.clear();
    driven_values.clear();

    AppendMonitorFrame(monitor_frame);
    AppendDriverFrame(driver_frame);
}


void test_lib::TestSequence::AppendDriverFrame(can::BitFrame& driver_frame)
{
    int bit_count = driver_frame.GetBitCount();
    can::Bit *bit;

    for (int i = 0; i < bit_count; i++)
    {
        bit = driver_frame.GetBit(i);
        AppendDriverBit(bit);
    }
}


void test_lib::TestSequence::AppendMonitorFrame(can::BitFrame& monitor_frame)
{
    int bit_count = monitor_frame.GetBitCount();
    can::Bit *bit;
    can::Bit *next_bit;

    for (int i = 0; i < bit_count; i++)
    {
        bit = monitor_frame.GetBit(i);

        // Ugly and low performing, I know, but we dont care here!
        if (i < bit_count - 1)
            next_bit = monitor_frame.GetBit(i + 1);

        if (bit->bit_type_ == can::BitType::Brs ||
            bit->bit_type_ == can::BitType::CrcDelimiter ||

            /* Whenever we transmitt error frame, we might switch bit-rate. Even if we
             * dont, then we calculate items as if bit-rate was switched.
             * "appendMonitorBitWithShift" deals calculates lenghts properly based on
             * what is inside the bit!
             */
            next_bit->bit_type_ == can::BitType::ActiveErrorFlag ||
            next_bit->bit_type_ == can::BitType::PassiveErrorFlag)
            appendMonitorBitWithShift(bit);
        else
            appendMonitorNotShift(bit);
    }
}


void test_lib::TestSequence::AppendDriverBit(can::Bit* bit)
{
    int time_quantas = bit->GetLengthTimeQuanta();
    can::BitValue bit_value = bit->bit_value_;
    can::BitValue last_value = bit_value;
    can::BitValue current_value;
    can::TimeQuanta *time_quanta;
    can::CycleBitValue* cycle_bit_value;
    std::chrono::nanoseconds duration (0);
    int cycles;

    for (int i = 0; i < time_quantas; i++)
    {
        time_quanta = bit->GetTimeQuanta(i);
        cycles = time_quanta->getLengthCycles();

        for (int j = 0; j < cycles; j++)
        {
            cycle_bit_value = time_quanta->getCycleBitValue(j);

            if (cycle_bit_value->has_default_value())
                current_value = bit_value;
            else
                current_value = cycle_bit_value->bit_value();

            // Note: This ignores non-default values which are equal to
            //       its default value (as expected) and merges them into
            //       single monitored / driven item!

            // If we did not detect bit value change -> it still belongs to the
            // same segment -> legnthen it
            if (current_value == last_value)
                duration += clock_period;

            // We detected value change or are at the end of bit, add item.
            if (current_value != last_value ||
                ((i == time_quantas - 1) && (j == cycles - 1))) 
            {
                // TODO: Push with message on first Item of bit
                // TODO: Push with message on each next item signalling glitch!
                
                if (duration.count() > 0)
                    pushDriverValue(duration, last_value, bit->GetBitTypeName());
                duration = clock_period;
                last_value = current_value;
            }
        }
    }
}

void test_lib::TestSequence::appendMonitorBitWithShift(can::Bit *bit)
{
    std::chrono::nanoseconds tseg_1_duration (0);
    std::chrono::nanoseconds tseg_2_duration (0);

    /* Count Tseg1 duration */
    size_t tseg_1_len = bit->GetPhaseLenTimeQuanta(can::BitPhase::Sync);
    tseg_1_len += bit->GetPhaseLenTimeQuanta(can::BitPhase::Prop);
    tseg_1_len += bit->GetPhaseLenTimeQuanta(can::BitPhase::Ph1);

    /* Tseg2 duration */
    size_t tseg_2_len = bit->GetPhaseLenTimeQuanta(can::BitPhase::Ph2);

    /* Count lenghts in nanoseconds */
    for (size_t i = 0; i < tseg_1_len; i++)
        for (size_t j = 0; j < bit->GetTimeQuanta(i)->getLengthCycles(); j++)
            tseg_1_duration += clock_period;

    for (size_t i = 0; i < tseg_2_len; i++)
        for (size_t j = 0; j < bit->GetTimeQuanta(can::BitPhase::Ph2, i)->getLengthCycles(); j++)
            tseg_2_duration += clock_period;

    /* Get sample rate for each phase! */
    size_t brp = bit->GetTimeQuanta(can::BitPhase::Sync, 0)->getLengthCycles();
    size_t brp_fd = bit->GetTimeQuanta(can::BitPhase::Ph2, 0)->getLengthCycles();
    std::chrono::nanoseconds sampleRateNominal = brp * clock_period;
    std::chrono::nanoseconds sampleRateData = brp_fd * clock_period;

    pushMonitorValue(tseg_1_duration, sampleRateNominal, bit->bit_value_, bit->GetBitTypeName());
    pushMonitorValue(tseg_2_duration, sampleRateData, bit->bit_value_, bit->GetBitTypeName());
}

void test_lib::TestSequence::appendMonitorNotShift(can::Bit *bit)
{
    std::chrono::nanoseconds duration (0);

    for (size_t i = 0; i < bit->GetLengthTimeQuanta(); i++)
        for (size_t j = 0; j < bit->GetTimeQuanta(i)->getLengthCycles(); j++)
            duration += clock_period;

    // Assume first Time quanta length is the same as rest (which is reasonable)!
    size_t brp = bit->GetTimeQuanta(0)->getLengthCycles();
    std::chrono::nanoseconds sample_rate = brp * clock_period;

    pushMonitorValue(duration, sample_rate, bit->bit_value_, bit->GetBitTypeName());
}


void test_lib::TestSequence::pushDriverValue(std::chrono::nanoseconds duration,
                                             can::BitValue bit_value,
                                             std::string message)
{
    // TODO: This conversion should ideally be separated!
    StdLogic logic_val;
    if (bit_value == can::BitValue::Dominant)
        logic_val = StdLogic::LOGIC_0;
    else
        logic_val = StdLogic::LOGIC_1;

    driven_values.push_back(DriverItem(duration, logic_val, message));
}


void test_lib::TestSequence::pushMonitorValue(std::chrono::nanoseconds duration,
                                              std::chrono::nanoseconds sample_rate,
                                              can::BitValue bit_value,
                                              std::string message)
{
    // TODO: This conversion should ideally be separated!
    StdLogic logic_val;
    if (bit_value == can::BitValue::Dominant)
        logic_val = StdLogic::LOGIC_0;
    else
        logic_val = StdLogic::LOGIC_1;

    monitored_values.push_back(MonitorItem(duration, logic_val, sample_rate, message));
}


void test_lib::TestSequence::PrintDrivenValues()
{
    for (auto driven_value : driven_values)
        driven_value.Print();
    std::cout << std::endl;
}


void test_lib::TestSequence::PrintMonitoredValues()
{
    for (auto monitored_value : monitored_values)
        monitored_value.Print();
    std::cout << std::endl;
}


void test_lib::TestSequence::PushDriverValuesToSimulator()
{
    for (auto driven_value : driven_values)
    {
        if (driven_value.HasMessage())
            CanAgentDriverPushItem((char)driven_value.value_, driven_value.duration_,
                                   driven_value.message_);
        else
            CanAgentDriverPushItem((char)driven_value.value_, driven_value.duration_);
    }
}


void test_lib::TestSequence::PushMonitorValuesToSimulator()
{
    for (auto monitorValue : monitored_values)
    {
        if (monitorValue.HasMessage())
            CanAgentMonitorPushItem((char)monitorValue.value_, monitorValue.duration_,
                                    monitorValue.sample_rate_, monitorValue.message_);
        else
            CanAgentMonitorPushItem((char)monitorValue.value_, monitorValue.duration_,
                                    monitorValue.sample_rate_);
    }
}