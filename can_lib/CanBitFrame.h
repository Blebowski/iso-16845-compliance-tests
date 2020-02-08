/*
 * TODO: License
 */

#include <cstdint>
#include <list>

#include "CanFrame.h"
#include "CanBit.h"

using namespace std;
using namespace can;

#ifndef CAN_BIT_FRAME
#define CAN_BIT_FRAME

class CanBitFrame : public CanFrame {

    public:
        CanBitFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                    RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                    ErrorStateIndicator isEsi, uint8_t dlc, int identifier,
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
        int insertStuffBits();
        
        /**
         * 
         */
        bool insertStuffCount();

        /**
         * 
         */
        uint32_t calculateCRC();

        /**
         * 
         */
        CanBit* getBit(int index);

        /**
         * 
         */
        CanBit* getBit(int index, BitType bitType);

        /**
         * 
         */
        int getBitIndex(CanBit canBit);

        /**
         * 
         */
        CanBit* getStuffBit(int index);

        /**
         * 
         */
        CanBit* getFixedStuffBit(int index);

        /**
         * 
         */
        bool insertBit(CanBit canBit, int index);

        /**
         * 
         */
        bool removeBit(CanBit *canBit);

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
        bool insertActiveErrorFrame(CanBit *canBit);

        /**
         * 
         */
        bool insertPassiveErrorFrame(int index);

        /**
         * 
         */
        bool insertPassiveErrorFrame(CanBit *canBit);

        /**
         * 
         */
        bool insertOverloadFrame(int index);

        /**
         * 
         */
        bool insertOverloadFrame(CanBit *canBit);

        /**
         * 
         */
        bool looseArbitration(int index);

        /**
         * 
         */
        bool looseArbitration(CanBit *canBit);

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
        void print();

        // TODO: We might consider having methods for setting DLC, FDF, IDE (etc...)
        //       which modify according fields of "bits_" list!

    private:
        std::list<CanBit> bits_;

        uint32_t crc15_;
        uint32_t crc17_;
        uint32_t crc21_;

        uint8_t stuffCount_;

        void push_bit(uint8_t bit_val, BitType bitType);
};

#endif