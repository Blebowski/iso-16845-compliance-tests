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
        int GetBitCount();

        /**
         * @param index Index of bit within frame to get (SOF = 0, first bit of Base ID = 1, ...)
         * @returns Pointer to bit on 'index' position, aborts if index is higher than number
         *          of bits in the frame.
         */
        Bit* GetBit(int index);

        /**
         * @param index Index of bit within frame to get (SOF = 0, first bit of Base ID = 1, ...)
         * @returns Iterator of bit on 'index' position, aborts if index is higher than number
         *          of bits in the frame.
         */
        std::list<Bit>::iterator GetBitIterator(int index);

        /**
         * Returns bit within given bit field. Stuff bits are counted too.
         * @param index Index of bit of given type within a bit field.
         * @param bit_type Type of bit (bit field)
         * @returns Pointer to bit on 'index' position within 'bit_type' field, aborts if 'bit_type'
         *          field is not existent or does not have enough bits.
         */
        Bit* GetBitOf(int index, BitType bit_type);

        /**
         * Returns bit within given bit field, but skip stuff bits. This function can be used
         * if e.g. you want to return 11 (last) bit of base identifier regardless of number of
         * stuff bits in the identifier.
         * @param index Index of bit of given type within a bit field.
         * @param bit_type Type of bit (bit field)
         * @returns Pointer to bit on 'index' position within 'bit_type' field, aborts if 'bit_type'
         *          field is not existent or does not have enough bits.
         */
        Bit* GetBitOfNoStuffBits(int index, BitType bit_type);

        /**
         * Returns bit within given bit field. Stuff bits are counted too.
         * @param index Index of bit of given type within a bit field.
         * @param bit_type Type of bit (bit field)
         * @returns Iterator to bit on 'index' position within 'bit_type' field.
         */
        std::list<Bit>::iterator GetBitOfIterator(int index, BitType bit_type);

        /**
         * Obtains bit index of bit within a frame.
         * @param can_bit Pointer to a bit (must be within a frame)
         * @returns Index of a bit within a frame (starting from 0 = SOF)
         */
        int GetBitIndex(Bit *can_bit);

        /**
         * Obtains stuff bit within a frame.
         * @param index Index of stuff bit within frame (0 - first stuff bit, 1 - second, ...)
         * @returns Pointer to stuff bit
         */
        Bit* GetStuffBit(int index);

        /**
         * Obtains stuff bit within a frame.
         * @param index Index of fixed stuff bit within frame (0 - first stuff bit, 1 - second, ...)
         * @returns Pointer to stuff bit
         */
        Bit* GetFixedStuffBit(int index);

        /**
         * Inserts bit to frame.
         * @param can_bit Bit to insert
         * @param index Position where bit shall be inserted. Bit existing on this index will be
         *              shited to one index higher.
         * @returns true if successfull, false otherwise.
         */
        bool InsertBit(Bit can_bit, int index);

        /**
         * Appends bit to a frame
         * @param can_bit Bit to append
         */
        void AppendBit(Bit can_bit);

        /**
         * Removes bit from frame.
         * @param can_bit Bit to remove from frame
         * @returns true if sucesfull, false if 'can_bit' is not from this frame.
         */
        bool RemoveBit(Bit *can_bit);

        /**
         * Removes bit from frame
         * @param index Index of bit to be removed.
         * @returns true if sucesfull, false if frame has less than 'index' + 1 bits.
         */
        bool RemoveBit(int index);

        /**
         * Removes bits from index till end of bit.
         * @param index Index from which to remove bits.
         * @returns true if sucesfull, false if frame has less than 'index' + 1 bits.
         */
        bool RemoveBitsFrom(int index);

        /**
         * Inserts Error Flag to a frame (Error Delimiter is not inserted).
         * @param index Index of a bit on which Error flag shall start.
         * @param error_flag_type Type of Error flag (active, passive)
         * @returns true if succesfull, false otherwise
         */
        bool InsertErrorFlag(int index, BitType error_flag_type);

        /**
         * Inserts Active Error frame to a frame. Emulates as if CAN controller detected error.
         * @param index index of a bit from which Active error frame shall be inserted.
         *              (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertActiveErrorFrame(int index);

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
        bool InsertPassiveErrorFrame(int index);

        /**
         * Inserts Passive Error frame to a frame. Emulates as if CAN controller detected error.
         * @param can_bit pointer to bit from which error frame shall be inserted.
         *               (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertPassiveErrorFrame(Bit *can_bit);

        /**
         * Inserts Overload frame to a frame. Emulates as if CAN controller detected overload
         * condition.
         * @param index index of a bit from which Overload frame shall be inserted.
         *              (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertOverloadFrame(int index);

        /**
         * Inserts Overload frame to a frame. Emulates as if CAN controller detected overload
         * condition.
         * @param can_bit pointer to bit from which overload frame shall be inserted.
         *               (This bit is effectively erased from frame).
         * @returns true if succesfull, false otherwise.
         */
        bool InsertOverloadFrame(Bit *can_bit);

        /**
         * Emulates node loosing arbitration by a CAN node. All bits after 'index' bit become
         * recessive. ACK bit becomes dominant. Arbitration can be lost only on bits which
         * belong to arbitration field.
         * @param index Index at which arbitration shall be lost.
         * @returns true if succesfull, false otherwise
         */
        bool LooseArbitration(int index);

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
         * ACK Slot is turned dominant.
         */
        void TurnReceivedFrame();

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
         * Recalculates all fields of frame (CRC, stuff bits, stuff count) which depend on
         * frame metadata (Data, Identifier, DLC, etc...). This method can be used to udpate
         * frame to have valid CRC after e.g. data bit was flipped.
         */
        void UpdateFrame();

    private:
        /* Bits within a frame */
        std::list<Bit> bits_;

        /* CRCs */
        uint32_t crc15_;
        uint32_t crc17_;
        uint32_t crc21_;

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
        bool ClearFrameBits(int index);

        /**
         * Calculates all necessary bit fields within CAN frame and creates bits of frame.
         */
        void BuildFrameBits();

        /**
         * Inserts stuff bits from first bit till start of Stuff count field (CAN FD frame).
         * In CAN 2.0 frame finish until the end of frame.
         * @returns number of stuff bits inserted.
         */
        int InsertNormalStuffBits();

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
         * Appends bit at the end of frame
         * @param bit_type type of bit to be appended
         * @param bit_val Bit value to be set
         */
        void AppendBit(BitType bit_type, BitValue bit_value);

        /**
         * Constructs bits of a frame from metadata.
         */
        void ConstructFrame();
};

#endif