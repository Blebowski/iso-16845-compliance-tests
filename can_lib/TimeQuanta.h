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
#include <cstdint>
#include <list>

#include "can.h"
#include "CycleBitValue.h"

#ifndef TIME_QUANTA
#define TIME_QUANTA


/**
 * @class TimeQuanta
 * @namespace can
 * 
 * Represents single Time Quanta. Contains individual cycles.
 */
class can::TimeQuanta
{
    public:

        /**
         *  @param brp Baud rate prescaler (number of cycles within Time quanta)
         *  @param bit_phase Phase of bit to which this time quanta belongs
         */
        TimeQuanta(Bit *parent, int brp, BitPhase bit_phase);

        /**
         * @param brp Baud rate prescaler (number of cycles within Time quanta)
         * @param bit_phase Phase of bit to which this time quanta belongs
         * @param bit_value Bit value of each cycle in time quanta (set as non default value)
         */
        TimeQuanta(Bit *parent, int brp, BitPhase bit_phase, BitValue bit_value);

        /**
         * @returns true if any of cycles in this Time quanta contain non-default values.
         */
        bool HasNonDefaultValues();

        /**
         * Sets all clock cycle values to default (Cycles with default values inherit value from
         * bit to which they belong).
         */
        void SetAllDefaultValues();

        /**
         * @returns length of Time quanta in clock cycles.
         */
        size_t getLengthCycles();

        /**
         * Gets value of cycle.
         * @param index Position of cycle within Time quanta.
         * @returns Pointer to cycle bit value.
         */
        CycleBitValue *getCycleBitValue(size_t index);

        /**
         * Gets value of cycle
         * @param index Position of cycle within Time Quanta.
         * @returns Iterator pointing to cycle bit value
         */
        std::list<CycleBitValue>::iterator GetCycleBitValueIterator(size_t index);

        /**
         * Lengthens time quanta (appends cycles at the end).
         * @param by_cycles number of cycles to lengthen by
         */
        void Lengthen(size_t by_cycles);

        /**
         * Lengthens time quanta (appends cycles at the end). Appended bits are forced to
         * non-default value.
         * @param by_cycles number of cycles to lengthen by
         * @param bit_value Value to set appended cycles to.
         */
        void Lengthen(size_t by_cycles, BitValue bit_value);

        /**
         * Shortens time quanta.
         * @param by_cycles number of cycles to shorten time quanta by.
         */
        void Shorten(size_t by_cycles);

        /**
         * Forces value of a one cycle.
         * @param cycle_index Index of cycle to force.
         * @param bit_value Value of cycle to force.
         * 
         * Function crashes if cycle_index is larger than number of cycles in Time Quanta.
         */
        void ForceCycleValue(size_t cycle_index, BitValue bit_value);

        /**
         * Forces value of each cycle within time quanta
         * @param bit_value Value to force time quanta to
         */
        void ForceValue(BitValue bit_value);

        /**
         * Phase of bit to which this time quanta belongs.
         */
        BitPhase bit_phase_;

    private:
        /* Cycle bit values within time quanta */
        std::list<CycleBitValue> cycle_bit_values_;

        /* Parent Bit which contains this Time Quanta*/
        Bit *parent_;
};

#endif