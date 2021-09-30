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

#include <cstdint>
#include <chrono>
#include <list>

#include "Frame.h"
#include "Bit.h"

#ifndef BIT_FRAME
#define BIT_FRAME


/**
 * @class Bit
 * @namespace can
 * 
 * Class representing single frame on CAN bus.
 * 
 */
class can::BitFrame : public Frame {

    public:
        BitFrame(FrameFlags frame_flags, uint8_t dlc, int identifier, uint8_t *data,
                 BitTiming* nominal_bit_timing, BitTiming* data_bit_timing);

        BitFrame(Frame &frame, BitTiming *nominal_bit_timing, BitTiming *data_bit_timing);

        /**
         * @returns number of bits within CAN frame
         */
        size_t GetBitCount();

        /**
         * @param bit_field Type of bit field whose length to find.
         * @returns Lenght of queried bit field within a frame. 0 if no such bit field exist.
         */
        size_t GetFieldLength(BitType bit_type);

        /**
         * @param index Index of bit within frame to get (SOF = 0, first bit of Base ID = 1, ...)
         * @returns Pointer to bit on 'index' position, aborts if index is higher than number
         *          of bits in the frame.
         */
        Bit* GetBit(size_t index);

        /**
         * @param index Index of bit within frame to get (SOF = 0, first bit of Base ID = 1, ...)
         * @returns Iterator of bit on 'index' position, aborts if index is higher than number
         *          of bits in the frame.
         */
        std::list<Bit>::iterator GetBitIterator(size_t index);

        /**
         * Returns bit within given bit field. Stuff bits are counted too.
         * @param index Index of bit of given type within a bit field.
         * @param bit_type Type of bit (bit field)
         * @returns Pointer to bit on 'index' position within 'bit_type' field, aborts if 'bit_type'
         *          field is not existent or does not have enough bits.
         */
        Bit* GetBitOf(size_t index, BitType bit_type);

        /**
         * Returns random bit within a Bit field
         * @param bit_type Type of bit (bit field)
         * @returns Pointer to random bit within the bit-field.
         */
        Bit* GetRandomBitOf(BitType bit_type);

        /**
         * Returns random bit within a frame with given value
         * @param bit_value Value of random bit
         * @returns Pointer to random bit within given value.
         */
        Bit* GetRandomBit(BitValue bit_value);

        /**
         * Returns bit within given bit field, but skip stuff bits. This function can be used
         * if e.g. you want to return 11 (last) bit of base identifier regardless of number of
         * stuff bits in the identifier.
         * @param index Index of bit of given type within a bit field.
         * @param bit_type Type of bit (bit field)
         * @returns Pointer to bit on 'index' position within 'bit_type' field, aborts if 'bit_type'
         *          field is not existent or does not have enough bits.
         */
        Bit* GetBitOfNoStuffBits(size_t index, BitType bit_type);

        /**
         * Returns bit within given bit field. Stuff bits are counted too.
         * @param index Index of bit of given type within a bit field.
         * @param bit_type Type of bit (bit field)
         * @returns Iterator to bit on 'index' position within 'bit_type' field.
         */
        std::list<Bit>::iterator GetBitOfIterator(size_t index, BitType bit_type);

        /**
         * Obtains bit index of bit within a frame.
         * @param can_bit Pointer to a bit (must be within a frame)
         * @returns Index of a bit within a frame (starting from 0 = SOF)
         */
        size_t GetBitIndex(Bit *can_bit);

        /**
         * Obtains stuff bit within a frame.
         * @param index Index of stuff bit within frame (0 - first stuff bit, 1 - second, ...)
         * @returns Pointer to stuff bit
         */
        Bit* GetStuffBit(int index);

        /**
         * Obtains stuff bit within a bit field of a frame
         * @param index Index of stuff bit within frame (0 - first stuff bit, 1 - second, ...)
         * @param bit_type Bit field to search stuff bit for
         * @returns Pointer to stuff bit
         */
        Bit* GetStuffBit(int index, BitType bit_type);

        /**
         * Obtains a Stuff bit within a bit field at certain position.
         * @param bit_type Type of bit field
         * @param stuff_bit_type Type of stuff bit to find (No stuff bit, Fixed, regular)
         * @param bit_value Consider bits of this value only
         * @returns Pointer to the bit
         */
        Bit* GetStuffBit(BitType bit_type, StuffBitType stuff_bit_type, BitValue bit_value);

        /**
         * Obtains fixed stuff bit within a frame.
         * @param index Index of fixed stuff bit within frame (0 - first stuff bit, 1 - second, ...)
         * @returns Pointer to stuff bit
         */
        Bit* GetFixedStuffBit(size_t index);

        /**
         * Obtains fixed stuff bit within a frame.
         * @param index Index of fixed stuff bit within frame (0 - first stuff bit, 1 - second, ...)
         * @param value Bit value to search for
         * @returns Pointer to fixed stuff bit with matching value on index position
         */
        Bit* GetFixedStuffBit(size_t index, BitValue bit_value);

        /**
         * Inserts bit to frame.
         * @param can_bit Bit to insert
         * @param index Position where bit shall be inserted. Bit existing on this index will be
         *              shited to one index higher.
         * @returns true if successfull, false otherwise.
         */
        bool InsertBit(Bit can_bit, size_t index);

        /**
         * Inserts bit to frame.
         * @param can_bit Bit to insert
         * @param index Position where bit shall be inserted. Bit existing on this index will be
         *              shited to one index higher.
         * @returns true if successfull, false otherwise.
         */
        bool InsertBit(BitType bit_type, BitValue bit_value, size_t index);

        /**
         * Appends bit to a frame.
         * @param can_bit Bit to append
         */
        void AppendBit(Bit can_bit);

        /**
         * Creates a new bit and appends it to a frame. Bit timing and frame flags are
         * inherited from frame itself.
         */
        void AppendBit(BitType bit_type, BitValue bit_value);

        /**
         * Removes bit from frame.
         * @param can_bit Bit to remove from frame
         * @returns true if sucesfull, false if 'can_bit' is not from this frame.
         */
        void RemoveBit(Bit *can_bit);

        /**
         * Removes bit from frame
         * @param index Index of bit to be removed.
         * @returns true if sucesfull, false if frame has less than 'index' + 1 bits.
         */
        bool RemoveBit(size_t index);

        /**
         * Removes bit from frame
         * @param index Index of bit within bit field to be removed.
         * @param bit_type Type of bit field to remove.
         */
        void RemoveBit(size_t index, BitType bit_type);

        /**
         * Removes bits from index till end of bit.
         * @param index Index from which to remove bits.
         * @returns true if sucesfull, false if frame has less than 'index' + 1 bits.
         */
        bool RemoveBitsFrom(size_t index);

        /**
         * Removes bits from bit index within bit field till end of bit.
         * @param index Index from which to remove bits.
         * @param bit_type Type of bit field to remove bits from
         */
        void RemoveBitsFrom(size_t index, BitType bit_type);

        /**
         * Inserts Error Flag to a frame (Error Delimiter is not inserted).
         * @param index Index of a bit on which Error flag shall start.
         * @param error_flag_type Type of Error flag (active, passive)
         * @returns true if succesfull, false otherwise
         */
        bool InsertErrorFlag(size_t index, BitType error_flag_type);

        /**
         * Inserts Active Error frame to a frame. Emulates as if CAN controller detected error.
         * @param index index of a bit from which Active error frame shall be inserted.
         *              (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertActiveErrorFrame(size_t index);

        /**
         * Inserts Active Error frame to specific bit within frame field of a frame.
         * @param index index of a bit (within frame field) at which Error frame shall start.
         * @param bit_type Type of bit (Frame field type)
         * @returns true if succesfull, false otherwise.
         */
        bool InsertActiveErrorFrame(size_t index, BitType bit_type);

        /**
         * Inserts Active Error frame to a frame. Emulates as if CAN controller detected error.
         * @param can_bit pointer to bit from which error frame shall be inserted.
         *               (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertActiveErrorFrame(Bit *can_bit);

        /**
         * Inserts Passive Error frame to a frame. Emulates as if CAN controller detected error.
         * @param index index of a bit from which Active error frame shall be inserted.
         *              (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertPassiveErrorFrame(size_t index);

        /**
         * Inserts Passive Error frame to a frame. Emulates as if CAN controller detected error.
         * @param can_bit pointer to bit from which error frame shall be inserted.
         *               (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertPassiveErrorFrame(Bit *can_bit);

        /**
         * Inserts Passive Error frame to specific bit within frame field of a frame.
         * @param index index of a bit (within frame field) at which Error frame shall start.
         * @param bit_type Type of bit (Frame field type)
         * @returns true if succesfull, false otherwise.
         */
        bool InsertPassiveErrorFrame(size_t index, BitType bit_type);

        /**
         * Inserts Overload frame to a frame. Emulates as if CAN controller detected overload
         * condition.
         * @param index index of a bit from which Overload frame shall be inserted.
         *              (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertOverloadFrame(size_t index);

        /**
         * Inserts Overload frame to a frame. Emulates as if CAN controller detected overload
         * condition.
         * @param can_bit pointer to bit from which overload frame shall be inserted.
         *               (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertOverloadFrame(Bit *can_bit);

        /**
         * Inserts Overload frame to specific bit within frame field of a frame.
         * @param index index of a bit (within frame field) at which Overload frame shall start.
         * @param bit_type Type of bit (Frame field type)
         * @returns true if succesfull, false otherwise.
         */
        bool InsertOverloadFrame(size_t index, BitType bit_type);

        /**
         * Appends 8 Suspend Transmission bits to after last bit in a frame.
         */
        void AppendSuspendTransmission();

        /**
         * Emulates node loosing arbitration by a CAN node. All bits after 'index' bit become
         * recessive. ACK bit becomes dominant. Arbitration can be lost only on bits which
         * belong to arbitration field.
         * @param index Index at which arbitration shall be lost.
         * @returns true if succesfull, false otherwise
         */
        bool LooseArbitration(size_t index);

        /**
         * Emulates node loosing arbitration by a CAN node. All bits after 'index' bit become
         * recessive. ACK bit becomes dominant. Arbitration can be lost only on bits which
         * belong to arbitration field.
         * @param index Index within field at which arbitration shall be lost.
         * @param bit_type Bit field at which arbitration shall be lost
         * @returns true if succesfull, false otherwise
         */
        bool LooseArbitration(size_t index, BitType bit_type);

        /**
         * Emulates node loosing arbitration by a CAN node. All bits after 'index' bit become
         * recessive. ACK bit becomes dominant. Arbitration can be lost only on bits which
         * belong to arbitration field.
         * @param can_bit Pointer to bit at which arbitration shall be lost.
         * @returns true if succesfull, false otherwise 
         */
        bool LooseArbitration(Bit *can_bit);

        /**
         * Converts frame as if this frame was received frame. All bits are turned Recessive.
         * ACK Slot is turned dominant. In FD frame only first bit of ACK slot bit is turned to
         * dominant since receiver shall send only one dominant ACK!
         */
        void TurnReceivedFrame();

        /**
         * Gets number of Stuff bits in a bit field of a frame.
         * @param bit_type Type of bit to find number of stuff bits in
         * @param stuff_bit_type Type of stuff bit to count (No stuff bit, Fixed, regular)
         * @returns Number of stuff bits within bit field of a frame.
         */
        int GetNumStuffBits(BitType bit_type, StuffBitType stuff_bit_type);

        /**
         * Gets number of Stuff bits in a bit field of a frame with matching value
         * @param bit_type Type of bit to find number of stuff bits in
         * @param stuff_bit_type Type of stuff bit to count (No stuff bit, Fixed, regular)
         * @param bit_value Value of bit to check
         * @returns Number of stuff bits within bit field of a frame matching required value
         */
        int GetNumStuffBits(BitType bit_type, StuffBitType stuff_bit_type, BitValue bit_value);

        /**
         * Gets number of Stuff bits in whole frame
         * @param stuff_bit_type Type of stuff bit to count (No stuff bit, Fixed, regular)
         * @returns Number of stuff bits within a frame.
         */
        int GetNumStuffBits(StuffBitType stuff_bit_type);

        /**
         * Gets number of Stuff bits with given value
         * @param stuff_bit_type Type of stuff bit to count (No stuff bit, Fixed, regular)
         * @param bit_value Consider only bits of this value
         * @returns Number of stuff bits within a frame.
         */
        int GetNumStuffBits(StuffBitType stuff_bit_type, BitValue bit_value);

        /**
         * @returns CRC of frame. Real CRC is returned based on frame type (CAN 2.0 or FD)!
         */
        uint32_t crc();

        /**
         * @returns Stuff count of frame.
         */
        uint8_t stuff_count();

        /**
         * @returns base part of identifier.
         */
        uint32_t base_identifier();

        /**
         * @returns Identifier extension part of identifier
         */
        uint32_t identifier_extension();

        /**
         * Appends another frame after the last bit of this frame.
         * @param bit_frame Frame to append
         */
        void AppendBitFrame(can::BitFrame *bit_frame);

        /**
         * Prints frame.
         * @param print_stuff_bits prints stuff bits if true, othewise stuff bits are skipped.
         */
        void Print(bool print_stuff_bits);

        /**
         * Prints frame with detailed timing information
         */
        void PrintDetailed(std::chrono::nanoseconds clock_period);

        /**
         * Updates frame. Following is done:
         *  1. Stuff bits are updated
         *  2. CRC is recalculated (if allowed).
         * 
         * This function can be used to update the frame to have valid CRC after a bit
         * was changed in it. Alternatively, it can be used to only re-stuff the frame
         * after CRC was corrupted.
         * 
         * @param recalc_crc When true, CRC will be recalculated.
         */
        void UpdateFrame(bool recalc_crc = true);

        /**
         * Moves back in frame in units of Cycle bit values (smallest fractions from which
         * TIme Quanta is built).
         * @param from Starting cycle to move from.
         * @param move_by Number of cycles to move by.
         * @returns Pointer to cycle which is 'move_by' before in frame, than 'from'.
         * 
         * E.g. if 'from' is first cycle of first bit of base id, and 'move_by' is equal to
         *      number of clock cycles per-bit time, then pointer to first cycle of SOF
         *      will be returned.
         */
        CycleBitValue *MoveCyclesBack(CycleBitValue *from, size_t move_by);

        /**
         * Compensates recessive to dominat transition within a bit to account for input delay
         * of IUT.
         * 
         * Rationale is following:
         *  If LT applies dominant bit exactly at start of bit, as is transmitted by IUT,
         *  then IUT will see this bit only 'input delay' later. If prescaler is small
         *  enough, this will cause synchronization edge to be seen by IUT already after
         *  SYNC segment. IUT will therefore execute positive resynchronization.
         *  Due to this, all subsequent monitored values will be shifted by an amount of
         *  this "parasitic" resynchronisation.
         * 
         *  To avoid this effect, N last cycles of previous bit need to be forced to dominant
         *  (N being IUTs input delay), so that IUT will see synchronization edge right
         *  in SYNC segment.
         * 
         * @param from Starting bit which must be transmitted Dominant by IUT.
         * @param input_delay Input delay of DUT in clock cycles.
         */
        void CompensateEdgeForInputDelay(Bit *from, int input_delay);

        /**
         * Flips bit value. If bit was flipped from Recessive to Dominant,then compensates
         * input delay of IUT.
         * @param bit Bit to be flipped
         * @param input_delay Input delay of iUT in clock cycles
         */
        void FlipBitAndCompensate(Bit *bit, int input_delay);

        /**
         * Acknowledges the frame (forces ACK low).
         */
        void PutAcknowledge();

        /**
         * Acknowledges the frame (forces ACK low), and compensates IUTs input delay
         * on ACK bit.
         */
        void PutAcknowledge(int input_delay);

    private:
        /* Bits within a frame */
        std::list<Bit> bits_;

        /* CRCs */
        uint32_t crc15_;
        uint32_t crc17_;
        uint32_t crc21_;
        size_t crc_lenght;

        /* Stuff counts: unsigned and grey coded */
        uint8_t stuff_count_;
        uint8_t stuff_count_encoded_;

        /* Bit timing - used to construct time quantas / cycles within bits of frame*/
        BitTiming* data_bit_timing_;
        BitTiming* nominal_bit_timing_;

        /**
         * Erases frame bits from index till end of frame.
         * @param index Index to clear bits from
         * @return true is succesfull, false otherwise (e.g. bit is not within frame)
         */
        bool ClearFrameBits(size_t index);

        /**
         * Calculates all necessary bit fields within CAN frame and creates bits of frame.
         */
        void BuildFrameBits();

        /**
         * Inserts stuff bits from first bit till start of Stuff count field (CAN FD frame).
         * In CAN 2.0 frame finish until the end of frame.
         * @returns number of stuff bits inserted.
         */
        size_t InsertNormalStuffBits();

        /**
         * Inserts stuff bits to stuff count field (first bit and stuff bit after parity).
         */
        void InsertStuffCountStuffBits();

        /**
         * Inserts fixed stuff bits to CRC field.
         */
        void InsertCrcFixedStuffBits();

        /**
         * Sets bits within stuff count field based on number of regular stuff bits.
         * @returns true if succesfull, false otherwise (e.g. stuff count bits not present)
         */
        bool SetStuffCount();

        /**
         * Sets stuff parity based on stuff count.
         * @returns True if sucessfull, false otherwise (e.g. stuff parity bit not present)
         */
        bool SetStuffParity();

        /**
         * Iterates through bits of a frame till CRC field and calculates CRC. CRC bits
         * are NOT set to value of calculated CRC.
         * @returns calculated CRC
         */
        uint32_t CalculateCrc();

        /**
         * Sets CRC bits to CRC value.
         */
        void UpdateCrcBits();

        /**
         * Prints bit which belongs to single bit field (SOF, IDE, RTR, etc...)
         * @param bit iterator of bit to be printed
         * @param vals Pointer to line with bit values
         * @param names Pointer to line with bit names
         * @param print_stuff_bits if true, stuff bits are included, if false, they are skipped.
         */
        void PrintSingleBitField(std::list<Bit>::iterator& bit,
                                 std::string *vals,
                                 std::string *names,
                                 bool print_stuff_bits);

        /**
         * Prints bit which belongs to multiple bit field (Data, CRC, etc...)
         * @param bit iterator of bit to be printed
         * @param vals Pointer to line with bit values
         * @param names Pointer to line with bit names
         * @param print_stuff_bits if true, stuff bits are included, if false, they are skipped.
         */
        void PrintMultiBitField(std::list<Bit>::iterator& bit,
                                std::string *vals,
                                std::string *names,
                                bool print_stuff_bits);

        /**
         * Appends bit at the end of frame
         * @param bit_type type of bit to be appended
         * @param bit_val Bit value to be set (0 - BitType:Dominant, 1 - BitType:Recessive)
         */
        void AppendBit(BitType bit_type, uint8_t bit_val);


        /**
         * Constructs bits of a frame from metadata.
         */
        void ConstructFrame();
};

#endif