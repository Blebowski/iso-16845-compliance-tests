#ifndef BIT_H
#define BIT_H
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
#include <cstdint>
#include <algorithm>
#include <list>
#include <string>
#include <assert.h>

#include "can.h"
#include "Cycle.h"
#include "FrameFlags.h"
#include "BitTiming.h"
#include "TimeQuanta.h"

/**
 * @class Bit
 * @namespace can
 *
 * Class representing single bit on CAN bus.
 *
 */
class can::Bit {

    public:
        Bit(BitFrame *bit_frame, BitKind kind, BitVal val, FrameFlags* frm_flags,
            BitTiming* nbt, BitTiming* dbt);

        Bit(BitFrame *bit_frame, BitKind kind, BitVal val, FrameFlags* frm_flags,
            BitTiming* nbt, BitTiming* dbt, StuffKind stuff_kind);

        /* Type of bit: SOF, Base Identifier, CRC, ACK, etc... */
        BitKind kind_;

        /* Value on CAN bus: Dominant, Recessive */
        BitVal val_;

        /* Type of stuff bit: No stuff bit, fixed, regular */
        StuffKind stuff_kind_;

        /**
         * Flips value of bit from CAN bus perspective.
         * Dominant -> turned to Recessive.
         * Recessive -> turned to Dominant.
         */
        void FlipVal();

        /**
         * Checks if bit is stuff bit.
         * @returns true    when bit is stuff bit (regular or fixed)
         *          false   otherwise.
         */
        bool IsStuffBit();

        /**
         * Checks if bit represents bit field which has single bit on CAN bus
         * (e.g SOF, IDE, EDL fields have single bit, DATA or CRC have multiple bits)
         * @return true     if bit field of this bit has single bit
         *         false    otherwise.
         */
        bool IsSingleBitField();

        /**
         * Gets value of bit as coloured string. Colour achieved by ANSI sequence
         *   @returns String with bit value coloured according to key:
         *      stuff bits - green
         *      error frame bits - red
         *      overload frame bits - light blue
         *      others - no added color
         */
        std::string GetColouredVal();

        /**
         * @returns String representation of Bit type (e.g BitType::SOF) -> "SOF".
         */
        std::string GetBitKindName();

        /**
         * @returns BitValue::Dominant if Bit is BitValue::Recessive and vice versa.
         */
        BitVal GetOppositeVal();

        /**
         * Checks whether bit contains Time quantas with a bit_phase type. By default,
         * bits contain time quantas of all bit-phases.
         * @param phase Phase to check.
         * @returns true    if bit contains bit_phase time quantas
         *          false   otherwise
         */
        bool HasPhase(BitPhase phase);

        /**
         * Checks if some of bits time quantas contain non-default bit value .
         * @returns true    if there is a time quanta with non-default values.
         *          false   if all time quantas contain only default values.
         */
        bool HasNonDefVals();

        /**
         * @param phase phase whose length to find out
         * @returns length of bit phase in time quantas
         *          (e.g. by default lenght of BitPhase::Sync is 1)
         */
        size_t GetPhaseLenTQ(BitPhase phase);

        /**
         * @param phase phase whose length to find out
         * @returns length of Bit phase in clock cycles
         */
        size_t GetPhaseLenCycles(BitPhase phase);

        /**
         * @returns Length of Bit in time quantas.
         */
        size_t GetLenTQ();

        /**
         * @returns Length of bit in clock cycles.
         */
        size_t GetLenCycles();

        /**
         * Shortens bit phase by number of Time quantas. If a phase is shortened
         * by more or equal to number of bits that it has, phase is effectively
         * removed.
         * @param phase phase to be shortened
         * @param n_tqs number of time quantas to shorten by
         * @returns Number of time quantas by which a phase was shortened.
         */
        size_t ShortenPhase(BitPhase phase, size_t n_tqs);

        /**
         * Lengthens phase by Number of time quantas. If the phase does not
         * exist, it is created at expected part of bit (e.g. if bit has no
         * "Prop" phase, it will be created between Sync and Phase 1).
         * @param phase phase to lengthen
         * @param n_tqs number of time quantas to lengthen the phase by
         */
        void LengthenPhase(BitPhase phase, size_t n_tqs);

        /**
         * @param index Index of time quanta to return (starting with 0)
         * @return Pointer to bit's time quanta on 'index' position.
         *
         * If there are less time quantas within a bit than 'index', aborts.
         */
        TimeQuanta* GetTQ(size_t index);

        /**
         * @param index Index of time quanta to return (starting with 0)
         * @return Iterator to bit's time quanta on 'index' position.
         *
         * If there are less time quantas within a bit than 'index', aborts.
         */
        std::list<TimeQuanta>::iterator GetTQIter(size_t index);

        /**
         * Gets Time Quanta within a bit phase.
         * @param phase phase whose Time Quantas shall be returned
         * @param index Index of time quanta (within bit phase) to return (starting with 0)
         * @return Pointer to time quanta on 'index' position within 'bit_phase'.
         *
         * If phase does not exist within a bit or bit does not have so many Time quanta, aborts.
         */
        TimeQuanta* GetTQ(BitPhase phase, size_t index);

        /**
         * Gets a cycle within a frame
         * @param index Index of cycle (within a frame) to return (starting with 0)
         * @return Pointer to cycle on 'index' position within frame.
         */
        Cycle* GetCycle(size_t index);

        /**
         * @return An index of cycle within bit
         */
        size_t GetCycleIndex(can::Cycle* cycle);

        /**
         * Forces a time quanta within a bit to value (Inserts a glitch).
         * @param index Time quanta to force (within bit)
         * @param bit_value Value to which Time quanta shall be forced
         * @return true if successfull, false otherwise
         */
        bool ForceTQ(size_t index, BitVal val);

        /**
         * Forces range of Time Quantas within a bit to value.
         * @param start Starting Time quanta index to force (within bit)
         * @param end Ending Time quanta index to force (within bit)
         * @param val Value to which Time quantas shall be forced
         * @return number of Time Quantas that has actually been forced.
         */
        size_t ForceTQ(size_t start, size_t end, BitVal val);

        /**
         * Forces Time Quanta within a bit phase of a bit to value.
         * @param index Time quanta index to force (within bit phase)
         * @param phase Bit phase to force
         * @param val Value to which Time quantas shall be forced
         * @return true is successfull, false otherwise
         */
        bool ForceTQ(size_t index, BitPhase phase, BitVal val);

        /**
         * Forces Time Quanta range within a bit phase of a bit to value.
         * @param start Starting Time quanta index to force (within bit phase)
         * @param end Ending Time quanta index to force (within bit phase)
         * @param phase Bit phase to force
         * @param val Value to which Time quantas shall be forced
         * @return number of Time Quantas that has actually been forced
         */
        size_t ForceTQ(size_t start, size_t end, BitPhase phase, BitVal val);

        /**
         * Corrects (re-calculates) length of Ph2 segment to Nominal bit rate.
         * This is applied if error frame is inserted after a bit in data bit rate.
         * Phase 2 of previous bit must be recomputed as if bit rate shift back
         * to nominal occured at sample point of previous bit!
         */
        void CorrectPh2LenToNominal();

        /**
         * @param phase whose previous phase to search for.
         * @return previous bit phase within a bit.
         * (e.g. PrevBitPhase(BitPhase::Phase1) returns BitPhase::Prop
         * If previous bit phase does not exists, it searches further until Sync phase
         */
        BitPhase PrevBitPhase(BitPhase phase);

        /**
         * @param phase whose next phase to search for.
         * @return next bit phase within a bit.
         * (e.g. NextBitPhase(BitPhase::Phase1) returns BitPhase:Ph2)
         * If next bit phase does not exist, it searches next bit phase until the end
         * of bit. If 'bit_phase' is last phase, returns 'bit_phase'
         */
        BitPhase NextBitPhase(BitPhase phase);

        /**
         * @param phase Bit phase whose bit rate to search for.
         * @return gets bit rate of given bit phase.
         */
        BitRate GetPhaseBitRate(BitPhase phase);

        /**
         * @param phase Bit phase whose bit timing structure to search for.
         * @return gets bit timing of given bit phase.
         */
        BitTiming* GetPhaseBitTiming(BitPhase phase);

        /**
         * @return iterator to first time quanta of a bit phase.
         */
        std::list<TimeQuanta>::iterator GetFirstTQIter(BitPhase phase);

        /**
         * @return iterator to last time quanta of a bit phase.
         */
        std::list<TimeQuanta>::iterator GetLastTQIter(BitPhase phase);

    protected:

        const BitKindName bit_kind_names_[30] =
        {
            {BitKind::Sof,                  "SOF"},
            {BitKind::BaseIdent,            "Base identifer"},
            {BitKind::ExtIdent,             "Extended identifier"},
            {BitKind::Rtr,                  "RTR"},
            {BitKind::Ide,                  "IDE"},
            {BitKind::Srr,                  "SRR"},
            {BitKind::Edl,                  "EDL"},
            {BitKind::R0,                   "R0 "},
            {BitKind::R1,                   "R1 "},
            {BitKind::Brs,                  "BRS"},
            {BitKind::Esi,                  "ESI"},
            {BitKind::Dlc,                  "DLC"},
            {BitKind::Data,                 "Data field"},
            {BitKind::StuffCnt,             "St.Ct."},
            {BitKind::StuffParity,          "STP"},
            {BitKind::Crc,                  "CRC"},
            {BitKind::CrcDelim,             "CRD"},
            {BitKind::Ack,                  "ACK"},
            {BitKind::AckDelim,             "ACD"},
            {BitKind::Eof,                  "End of Frame"},
            {BitKind::Interm,               "Intermission"},
            {BitKind::Idle,                 "Idle"},
            {BitKind::SuspTrans,            "Suspend"},
            {BitKind::ActErrFlag,           "Active Error flag"},
            {BitKind::PasErrFlag,           "Passive Error flag"},
            {BitKind::ErrDelim,             "Error delimiter"},
            {BitKind::OvrlFlag,             "Overload flag"},
            {BitKind::OvrlDelim,            "Overload delimiter"},
            {BitKind::Undefined,            "-"}
        };

        /**
         * Parent frame which contains this bit
         */
        BitFrame *parent_;

        /*
         * These hold information about bit timing and fact whether Bit-rate has
         * shifted so they are important for manipulation of bit cycles!
         *
         * Should be provided during creation of Bit and are not copied internally!
         */
        FrameFlags* frm_flags_;
        BitTiming* nbt_;
        BitTiming* dbt_;

        /**
         * Time quantas within the bit.
         */
        std::list<TimeQuanta> tqs_;

        /**
         * Constructs time quantas from timing information. Called upon bit creation.
         */
        void ConstructTQs();

        /** Default bit-phases present in each bit */
        static const BitPhase def_bit_phases[];
};

#endif