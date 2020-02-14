/*
 * TODO: License
 */

#include <cstdint>
#include <list>

#include "Frame.h"
#include "Bit.h"

#ifndef BIT_FRAME
#define BIT_FRAME

class can::BitFrame : public Frame {

    public:
        BitFrame(FrameFlags frameFlags, uint8_t dlc, int identifier,
                 uint8_t *data);

        /**
         * 
         */
        void buildFrameBits();

        /**
         * 
         */
        void clearFrameBits();

        /**
         * 
         */
        bool clearFrameBits(int index);

        /**
         * 
         */
        int insertNormalStuffBits();

        /**
         * 
         */
        bool insertStuffCountStuffBits();

        /**
         * 
         */
        void insertCrcFixedStuffBits();

        /**
         * 
         */
        bool setStuffCount();

        /**
         * 
         */
        bool setStuffParity();

        /**
         * 
         */
        uint32_t calculateCrc();

        /**
         * 
         */
        uint32_t setCrc();

        /**
         * 
         */
        Bit* getBit(int index);
        
        /**
         * 
         */
        std::list<Bit>::iterator getBitIterator(int index);
        
        /**
         * 
         */
        Bit* getBitOf(int index, BitType bitType);

        /**
         * 
         */
        std::list<Bit>::iterator getBitOfIterator(int index, BitType bitType);

        /**
         * 
         */
        int getBitIndex(Bit *canBit);

        /**
         * 
         */
        Bit* getStuffBit(int index);

        /**
         * 
         */
        Bit* getFixedStuffBit(int index);

        /**
         * 
         */
        bool insertBit(Bit canBit, int index);

        /**
         * 
         */
        bool removeBit(Bit *canBit);

        /**
         * 
         */
        bool removeBit(int index);

        /**
         * 
         */
        bool insertAck();

        /**
         * 
         */
        bool insertActiveErrorFrame(int index);

        /**
         * 
         */
        bool insertActiveErrorFrame(Bit *canBit);

        /**
         * 
         */
        bool insertPassiveErrorFrame(int index);

        /**
         * 
         */
        bool insertPassiveErrorFrame(Bit *canBit);

        /**
         * 
         */
        bool insertOverloadFrame(int index);

        /**
         * 
         */
        bool insertOverloadFrame(Bit *canBit);

        /**
         * 
         */
        bool looseArbitration(int index);

        /**
         * 
         */
        bool looseArbitration(Bit *canBit);

        /**
         * 
         */
        uint32_t getCrc();

        /**
         * 
         */
        uint8_t getStuffCount();

        /**
         * 
         */
        uint32_t getBaseIdentifier();

        /**
         * 
         */
        uint32_t getIdentifierExtension();

        /**
         * 
         */
        void print(bool printStuffBits);

        // TODO: We might consider having methods for setting DLC, FDF, IDE (etc...)
        //       which modify according fields of "bits_" list!

    private:
        std::list<Bit> bits_;

        uint32_t crc15_;
        uint32_t crc17_;
        uint32_t crc21_;

        uint8_t stuffCount_;
        uint8_t stuffCountEncoded_;

        /**
         * 
         */
        void printSingleBitField(std::list<Bit>::iterator& bit,
                                 std::string *vals,
                                 std::string *names,
                                 bool printStuffBits);

        /**
         * 
         */
        void printMultiBitField(std::list<Bit>::iterator& bit,
                                std::string *vals,
                                std::string *names,
                                bool printStuffBits);

        /**
         * 
         */
        void printMultiBitField(std::list<Bit>::iterator &startBit);

        void push_bit(uint8_t bit_val, BitType bitType);
};

#endif