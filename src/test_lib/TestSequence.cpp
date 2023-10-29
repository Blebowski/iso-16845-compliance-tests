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
#include <iomanip>

#include "TestSequence.h"

test::TestSequence::TestSequence(std::chrono::nanoseconds clock_period)
{
    this->clock_period = clock_period;
}


test::TestSequence::TestSequence(std::chrono::nanoseconds clock_period,
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


test::TestSequence::TestSequence(std::chrono::nanoseconds clock_period,
                                 can::BitFrame& driver_frame,
                                 can::BitFrame& monitor_frame)
{
    this->clock_period = clock_period;
    monitored_values.clear();
    driven_values.clear();

    AppendMonitorFrame(monitor_frame);
    AppendDriverFrame(driver_frame);
}


void test::TestSequence::AppendDriverFrame(can::BitFrame& driver_frame)
{
    int bit_count = driver_frame.GetLen();
    can::Bit *bit;

    for (int i = 0; i < bit_count; i++)
    {
        bit = driver_frame.GetBit(i);
        AppendDriverBit(bit);
    }
}


void test::TestSequence::AppendMonitorFrame(can::BitFrame& monitor_frame)
{
    int bit_count = monitor_frame.GetLen();
    can::Bit *bit;
    can::Bit *next_bit;

    for (int i = 0; i < bit_count; i++)
    {
        bit = monitor_frame.GetBit(i);

        // Ugly and low performing, I know, but we dont care here!
        if (i < bit_count - 1)
            next_bit = monitor_frame.GetBit(i + 1);

        if (bit->kind_ == can::BitKind::Brs ||
            bit->kind_ == can::BitKind::CrcDelim ||

            /* Whenever we transmitt error frame, we might switch bit-rate. Even if we
             * dont, then we calculate items as if bit-rate was switched.
             * "appendMonitorBitWithShift" deals calculates lenghts properly based on
             * what is inside the bit!
             */
            next_bit->kind_ == can::BitKind::ActErrFlag ||
            next_bit->kind_ == can::BitKind::PasErrFlag)
            appendMonitorBitWithShift(bit);
        else
            appendMonitorNotShift(bit);
    }
}


void test::TestSequence::AppendDriverBit(can::Bit* bit)
{
    int time_quantas = bit->GetLenTQ();
    can::BitVal bit_val = bit->val_;
    can::BitVal last_val = bit_val;
    can::BitVal curr_val;
    can::TimeQuanta *time_quanta;
    can::Cycle* cycle;
    std::chrono::nanoseconds duration (0);
    int cycles;

    for (int i = 0; i < time_quantas; i++)
    {
        time_quanta = bit->GetTQ(i);
        cycles = time_quanta->getLengthCycles();

        for (int j = 0; j < cycles; j++)
        {
            cycle = time_quanta->getCycleBitValue(j);

            // Obtain value of current cycle
            // Note: This ignores non-default values which are equal to
            //       its default value (as expected) and merges them into
            //       single monitored / driven item!
            if (cycle->has_def_val())
                curr_val = bit_val;
            else
                curr_val = cycle->bit_val();

            // Did not detect bit value change -> it still belongs to the same segment
            // -> legnthen it
            if (curr_val == last_val)
                duration += clock_period;

            // Detected value change, push previous item
            if (curr_val != last_val)
            {
                pushDriverValue(duration, last_val, bit->GetBitKindName());

                duration = clock_period;
                last_val = curr_val;
            }

            // Reach last cycle of bit, push rest
            if ((i == time_quantas - 1) && (j == cycles - 1))
            {
                pushDriverValue(duration, last_val, bit->GetBitKindName());
            }
        }
    }
}

void test::TestSequence::appendMonitorBitWithShift(can::Bit *bit)
{
    std::chrono::nanoseconds tseg_1_duration (0);
    std::chrono::nanoseconds tseg_2_duration (0);

    // Currently this function does not support translation with forcing on Bits
    // with Bit-rate shift, check it!
    for (size_t i = 0; i < bit->GetLenTQ(); i++)
        for (size_t j = 0; j < bit->GetTQ(i)->getLengthCycles(); j++)
            assert(bit->GetTQ(i)->getCycleBitValue(j)->has_def_val()
                    && "Forcing not supported on Bits with Bit-rate shift!");

    /* Count Tseg1 duration */
    size_t tseg_1_len = bit->GetPhaseLenTQ(can::BitPhase::Sync);
    tseg_1_len += bit->GetPhaseLenTQ(can::BitPhase::Prop);
    tseg_1_len += bit->GetPhaseLenTQ(can::BitPhase::Ph1);

    /* Count Tseg2 duration */
    size_t tseg_2_len = bit->GetPhaseLenTQ(can::BitPhase::Ph2);

    /* Count lenghts in nanoseconds */
    for (size_t i = 0; i < tseg_1_len; i++)
        for (size_t j = 0; j < bit->GetTQ(i)->getLengthCycles(); j++)
            tseg_1_duration += clock_period;

    for (size_t i = 0; i < tseg_2_len; i++)
        for (size_t j = 0; j < bit->GetTQ(can::BitPhase::Ph2, i)->getLengthCycles(); j++)
            tseg_2_duration += clock_period;

    // Get sample rate for each phase and push monitor item. Push only if the phase has non-zero
    // lenght. No need to monitor time sequences with 0 duration. Also, if e.g TSEG2 is 0 due
    // to its shortening in the test, we would not be able to query its Time Quanta 0!
    if (tseg_1_duration > std::chrono::nanoseconds(0))
    {
        size_t brp = bit->GetTQ(can::BitPhase::Sync, 0)->getLengthCycles();
        std::chrono::nanoseconds sampleRateNominal = brp * clock_period;
        pushMonitorValue(tseg_1_duration, sampleRateNominal, bit->val_, bit->GetBitKindName());
    }

    if (tseg_2_duration > std::chrono::nanoseconds(0))
    {
        size_t brp_fd = bit->GetTQ(can::BitPhase::Ph2, 0)->getLengthCycles();
        std::chrono::nanoseconds sampleRateData = brp_fd * clock_period;
        pushMonitorValue(tseg_2_duration, sampleRateData, bit->val_, bit->GetBitKindName());
    }
}

void test::TestSequence::appendMonitorNotShift(can::Bit *bit)
{
    int time_quantas = bit->GetLenTQ();
    can::BitVal bit_val = bit->val_;
    can::BitVal last_value = bit_val;
    can::BitVal current_value;
    can::TimeQuanta *time_quanta;
    can::Cycle* cycle;
    std::chrono::nanoseconds duration (0);
    int cycles;

    // Assume first Time quanta length is the same as rest (which is reasonable)!
    size_t brp = bit->GetTQ(0)->getLengthCycles();
    std::chrono::nanoseconds sample_rate = brp * clock_period;

    for (int i = 0; i < time_quantas; i++)
    {
        time_quanta = bit->GetTQ(i);
        cycles = time_quanta->getLengthCycles();

        for (int j = 0; j < cycles; j++)
        {
            cycle = time_quanta->getCycleBitValue(j);

            // Obtain value of current cycle
            // Note: This ignores non-default values which are equal to
            //       its default value (as expected) and merges them into
            //       single monitored / driven item!
            if (cycle->has_def_val())
                current_value = bit_val;
            else
                current_value = cycle->bit_val();

            // Did not detect bit value change -> it still belongs to the same segment
            // -> legnthen it
            if (current_value == last_value)
                duration += clock_period;

            // Detected value change, push previous item
            if (current_value != last_value)
            {
                pushMonitorValue(duration, sample_rate, last_value, bit->GetBitKindName());

                duration = clock_period;
                last_value = current_value;
            }

            // Reach last cycle of bit, push rest
            if ((i == time_quantas - 1) && (j == cycles - 1))
            {
                pushMonitorValue(duration, sample_rate, last_value, bit->GetBitKindName());
            }
        }
    }
}


void test::TestSequence::pushDriverValue(std::chrono::nanoseconds duration,
                                             can::BitVal bit_value,
                                             std::string message)
{
    StdLogic logic_val;
    if (bit_value == can::BitVal::Dominant)
        logic_val = StdLogic::LOGIC_0;
    else
        logic_val = StdLogic::LOGIC_1;

    driven_values.push_back(DrvItem(duration, logic_val, message));
}


void test::TestSequence::pushMonitorValue(std::chrono::nanoseconds duration,
                                              std::chrono::nanoseconds sample_rate,
                                              can::BitVal bit_value,
                                              std::string message)
{
    StdLogic logic_val;
    if (bit_value == can::BitVal::Dominant)
        logic_val = StdLogic::LOGIC_0;
    else
        logic_val = StdLogic::LOGIC_1;

    monitored_values.push_back(MonItem(duration, logic_val, sample_rate, message));
}


void test::TestSequence::PrintDrivenValues()
{
    for (auto driven_value : driven_values)
        driven_value.Print();
    std::cout << std::endl;
}


void test::TestSequence::PrintMonitoredValues()
{
    for (auto monitored_value : monitored_values)
        monitored_value.Print();
    std::cout << std::endl;
}


void test::TestSequence::PushDriverValuesToSimulator()
{
    for (auto &tmp : driven_values)
    {
        if (tmp.HasMessage())
            CanAgentDriverPushItem((char)tmp.value_, tmp.duration_, tmp.message_);
        else
            CanAgentDriverPushItem((char)tmp.value_, tmp.duration_);
    }
}


void test::TestSequence::PushMonitorValuesToSimulator()
{
    for (auto &tmp : monitored_values)
    {
        if (tmp.HasMessage())
            CanAgentMonitorPushItem((char)tmp.value_, tmp.duration_,
                                    tmp.sample_rate_, tmp.message_);
        else
            CanAgentMonitorPushItem((char)tmp.value_, tmp.duration_,
                                    tmp.sample_rate_);
    }
}

void test::TestSequence::Print(bool driven)
{
    std::cout
        << std::setw (20) << "Field"
        << std::setw (20) << "Value"
        << std::setw (20) << "Duration (ns)"<< std::endl;

    if (driven) {
        for (auto &tmp : driven_values)
            tmp.Print();
    } else {
        for (auto &tmp : monitored_values)
            tmp.Print();
    }

}