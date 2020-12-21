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
#include <algorithm>
#include <list>
#include <string>
#include <assert.h>

#include "can.h"
#include "CycleBitValue.h"
#include "FrameFlags.h"
#include "BitTiming.h"
#include "TimeQuanta.h"


#ifndef BIT
#define BIT


/**
 * @class Bit
 * @namespace can
 * 
 * Class representing single bit on CAN bus.
 * 
 */
class can::Bit {

    public:
        Bit(BitType bit_type, BitValue bit_value, FrameFlags* frame_flags,
            BitTiming* nominal_bit_timing, BitTiming* data_bit_timing);

        Bit(BitType bit_type, BitValue bit_value, FrameFlags* frame_flags,
            BitTiming* nominal_bit_timing, BitTiming* data_bit_timing,
            StuffBitType stuff_bit_type);

        /* Type of bit: SOF, Base Identifier, CRC, ACK, etc... */
        BitType bit_type_;

        /* Type of stuff bit: DontShift stuff bit, fixed, regular */
        StuffBitType stuff_bit_type;
        
        /* Value on CAN bus: Dominant, Recessive */
        BitValue bit_value_;

        /**
         * Flips value of bit from CAN bus perspective.
         * Dominant -> turned to Recessive.
         * Recessive -> turned to Dominant.
         */
        void FlipBitValue();

        /**
         * Checks whether bit is stuff bit.
         * @returns true when bit is stuff bit (regular or fixed), false otherwise.
         */
        bool IsStuffBit();

        /**
         * Checks if bit represents bit field which has single bit on CAN bus
         * (e.g SOF, IDE, EDL fields have single bit, DATA or CRC not)
         * @return true if bit field of this bit has single bit, false otherwise.
         */
        bool IsSingleBitField();

        /**
         * Gets string bit value (0,1) coloured.
         *   @returns stuff bits - green
         *            error frame bits - red
         *            overload frame bits - yellow
         *            others - white
         */
        std::string GetColouredValue();

        /**
         * @returns String representation of Bit type (e.g BitType::SOF) -> "SOF".
         */
        std::string GetBitTypeName();

        /**
         * @returns BitValue::Dominant if Bit is BitValue::Recessive and vice versa.
         */
        BitValue GetOppositeValue();

        /**
         * Checks whether bit contains Bit phase of interest.
         * @param bit_phase Phase to check.
         * @returns true if yes, false otherwise
         */
        bool HasPhase(BitPhase bit_phase);

        /**
         * Checks whether in some of bits time quantas contain non-default bit
         * value (glitch).
         * @returns true if there are non-default values.
         *         false, if all Time quantas contain only default values.
         */
        bool HasNonDefaultValues();

        /**
         * Sets all time quantas to have the same value as is value of this bit.
         */
        void SetAllDefaultValues();

        /**
         * Gets length of certain bit phase in Time quantas.
         * (e.g. for typial bit: getPhaceLenTimeQuanta(BitPhase::Sof) returns 1)
         * @param bit_phase phase whose length to find out
         * @returns length of bit in Time Quantas
         */
        size_t GetPhaseLenTimeQuanta(BitPhase bit_phase);

        /**
         * Gets length of bit phase in clock cycles.
         * @param bit_phase phase whose length to find out
         * @returns length of Bit phase in clock cycles
         */
        size_t GetPhaseLenCycles(BitPhase bit_phase);

        /**
         * @returns Overall bit length in Time quantas.
         */
        size_t GetLengthTimeQuanta();

        /**
         * @returns Overall bit length in clock cycles.
         */
        size_t GetLengthCycles();

        /**
         * Shortens bit phase by number of Time quantas. If a phase is shortened
         * by more or equal to number of bits that it has, phase is effectively
         * removed.
         * @param bit_phase phase to be shortened
         * @param num_time_quantas number of time quantas to shorten by
         * @returns Number of time quantas by which a phase was shortened.
         */
        size_t ShortenPhase(BitPhase bit_phase, size_t num_time_quantas);

        /**
         * Lengthens phase by Number of time quantas. If the phase does not
         * exist, it is created at expected part of bit (e.g. if bit has no
         * "Prop" phase, it will be created between Sync and Phase 1).
         * @param bit_phase phase to lengthen
         * @param num_time_quantas number of time quantas to lengthen the phase by
         */
        void LengthenPhase(BitPhase bit_phase, size_t num_time_quantas);

        /**
         * @param index Index of time quanta to return (starting with 0)
         * @return Pointer to bit's time quanta on 'index' position.
         * 
         * If there are less time quantas within a bit than 'index', aborts.
         */
        TimeQuanta* GetTimeQuanta(size_t index);

        /**
         * @param index Index of time quanta to return (starting with 0)
         * @return Iterator to bit's time quanta on 'index' position.
         * 
         * If there are less time quantas within a bit than 'index', aborts.
         */
        std::list<TimeQuanta>::iterator GetTimeQuantaIterator(size_t index);

        /**
         * Gets Time Quanta within a bit phase.
         * @param bit_phase phase whose Time Quantas shall be returned
         * @param index Index of time quanta (within bit phase) to return (starting with 0)
         * @return Pointer to time quanta on 'index' position within 'bit_phase'.
         * 
         * If phase does not exist within a bit or bit does not have so many Time quanta, aborts.
         */
        TimeQuanta* GetTimeQuanta(BitPhase bit_phase, size_t index);

        /**
         * Forces a time quanta within a bit to value (Inserts a glitch).
         * @param index Time quanta to force (within bit)
         * @param bit_value Value to which Time quanta shall be forced
         * @return true if successfull, false otherwise
         */
        bool ForceTimeQuanta(size_t index, BitValue bit_value);

        /**
         * Forces range of Time Quantas within a bit to value.
         * @param start_index Starting Time quanta index to force (within bit)
         * @param end_index Ending Time quanta index to force (within bit)
         * @param bit_value Value to which Time quantas shall be forced
         * @return number of Time Quantas that has actually been forced.
         */
        size_t ForceTimeQuanta(size_t start_index, size_t end_index, BitValue bit_value);

        /**
         * Forces Time Quanta within a bit phase of a bit to value.
         * @param index Time quanta index to force (within bit phase)
         * @param bit_phase Bit phase to force
         * @param bit_value Value to which Time quantas shall be forced
         * @return true is successfull, false otherwise
         */
        bool ForceTimeQuanta(size_t index, BitPhase bit_phase, BitValue bit_value);

        /**
         * Forces Time Quanta range within a bit phase of a bit to value. 
         * @param start_index Starting Time quanta index to force (within bit phase)
         * @param end_index Ending Time quanta index to force (within bit phase)
         * @param bit_value Value to which Time quantas shall be forced
         * @return number of Time Quantas that has actually been forced
         */
        size_t ForceTimeQuanta(size_t start_index, size_t end_index, BitPhase bit_phase,
                               BitValue bit_value);

        /**
         * Corrects (re-calculates) length of Ph2 segment to Nominal bit rate.
         * This is applied if error frame is inserted after a bit in data bit rate.
         * Phase 2 of previous bit must be recomputed as if bit rate shift back
         * to nominal occured at sample point of previous bit!
         */
        void CorrectPh2LenToNominal();

        /**
         * @param bit_phase whose previous phase to search for.
         * @return previous bit phase within a bit.
         * (e.g. PrevBitPhase(BitPhase::Phase1) returns BitPhase::Prop
         * If previous bit phase does not exists, it searches further until Sync phase
         */
        BitPhase PrevBitPhase(BitPhase bit_phase);

        /**
         * @param bit_phase whose next phase to search for.
         * @return next bit phase within a bit.
         * (e.g. NextBitPhase(BitPhase::Phase1) returns BitPhase:Ph2)
         * If next bit phase does not exist, it searches next bit phase until the end
         * of bit. If 'bit_phase' is last phase, returns 'bit_phase'
         */
        BitPhase NextBitPhase(BitPhase bit_phase);

        /**
         * @param bit_phase Bit phase whose bit rate to search for.
         * @return gets bit rate of given bit phase.
         */
        BitRate GetPhaseBitRate(BitPhase bit_phase);

        /**
         * @param bit_phase Bit phase whose bit timing structure to search for.
         * @return gets bit timing of given bit phase.
         */
        BitTiming* GetPhaseBitTiming(BitPhase bit_phase);

        /**
         * @return iterator to first time quanta of a bit phase.
         */
        std::list<TimeQuanta>::iterator GetFirstTimeQuantaIterator(BitPhase bit_phase);

        /**
         * @return iterator to last time quanta of a bit phase.
         */
        std::list<TimeQuanta>::iterator GetLastTimeQuantaIterator(BitPhase bit_phase);

    protected:

        /**
         * 
         */
        const BitTypeName bit_type_names_[29] =
        {
            {BitType::Sof,                  "SOF"},
            {BitType::BaseIdentifier,       "Base identifer"},
            {BitType::IdentifierExtension,  "Extended identifier"},
            {BitType::Rtr,                  "RTR"},
            {BitType::Ide,                  "IDE"},
            {BitType::Srr,                  "SRR"},
            {BitType::Edl,                  "EDL"},
            {BitType::R0,                   "R0 "},
            {BitType::R1,                   "R1 "},
            {BitType::Brs,                  "BRS"},
            {BitType::Esi,                  "ESI"},
            {BitType::Dlc,                  "DLC"},
            {BitType::Data,                 "Data field"},
            {BitType::StuffCount,           "St.Ct."},
            {BitType::StuffParity,          "STP"},
            {BitType::Crc,                  "CRC"},
            {BitType::CrcDelimiter,         "CRD"},
            {BitType::Ack,                  "ACK"},
            {BitType::AckDelimiter,         "ACD"},
            {BitType::Eof,                  "End of Frame"},
            {BitType::Intermission,         "Intermission"},
            {BitType::Idle,                 "Idle"},
            {BitType::Suspend,              "Suspend"},
            {BitType::ActiveErrorFlag,      "Active Error flag"},
            {BitType::PassiveErrorFlag,     "Passive Error flag"},
            {BitType::ErrorDelimiter,       "Error delimiter"},
            {BitType::OverloadFlag,         "Overload flag"},
            {BitType::OverloadDelimiter,    "Overload delimiter"}
        };

        /* 
         * These hold information about bit timing and fact whether Bit-rate has
         * shifted so they are important for manipulation of bit cycles!
         * 
         * Should be provided during creation of Bit and are not copied internally!
         */
        BitTiming* nominal_bit_timing;
        BitTiming* data_bit_timing;
        FrameFlags* frame_flags;

        /**
         * Time quantas within the bit.
         */
        std::list<TimeQuanta> time_quantas_;

        /**
         * Constructs time quantas from timing information. Called upon bit creation.
         */
        void ConstructTimeQuantas();
};

#endif