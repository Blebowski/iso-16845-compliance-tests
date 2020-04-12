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
#include <assert.h>

#include "can.h"
#include "CycleBitValue.h"
#include "FrameFlags.h"
#include "BitTiming.h"
#include "TimeQuanta.h"


#ifndef BIT
#define BIT

class can::Bit {

    public:
        Bit(BitType bitType, BitValue bitValue, FrameFlags* frameFlags,
            BitTiming* nominalBitTiming, BitTiming* dataBitTiming);

        Bit(BitType bitType, BitValue bitValue, FrameFlags* frameFlags,
            BitTiming* nominalBitTiming, BitTiming* dataBitTiming,
            StuffBitType stuffBitType);

        /**
         *
         */
        bool setBitValue(BitValue bitValue);

        /**
         *
         */
        BitValue getBitValue();

        /**
         *
         */
        BitType getBitType();

        /**
         * 
         */
        StuffBitType getStuffBitType();

        /**
         *
         */
        void flipBitValue();

        /**
         *
         */
        bool isStuffBit();

        /**
         *
         */
        bool isSingleBitField();

        /**
         *
         */
        std::string getStringValue();

        /**
         *
         */
        std::string getBitTypeName();

        /**
         *
         */
        BitValue getOppositeValue();

        /**
         *
         */
        bool hasPhase(BitPhase bitPhase);

        /*
         *
         */
        bool hasNonDefaultValues();

        /*
         *
         */
        void setAllDefaultValues();

        /*
         *
         */
        int getPhaseLenTimeQuanta(BitPhase bitPhase);

        /*
         *
         */
        int getPhaseLenCycles(BitPhase bitPhase);

        /*
         *
         */
        int getLenTimeQuanta();

        /*
         *
         */
        int getLenCycles();

        /**
         *
         */
        int shortenPhase(BitPhase bitPhase, int numTimeQuanta);

        /**
         *
         */
        void lengthenPhase(BitPhase bitPhase, int numTimeQuanta);

        /**
         *
         */
        TimeQuanta* getTimeQuanta(int index);

        /**
         *
         */
        TimeQuanta* getTimeQuanta(BitPhase bitPhase, int index);

        /*
         * 
         */
        bool forceTimeQuanta(int index, BitValue bitValue);

        /*
         * 
         */
        int forceTimeQuanta(int fromIndex, int toIndex, BitValue bitValue);

        /*
         * 
         */
        bool forceTimeQuanta(int index, BitPhase bitPhase, BitValue bitValue);

        /*
         * 
         */
        bool forceTimeQuanta(int fromIndex, int toIndex, BitPhase bitPhase,
                             BitValue bitValue);

        /**
         * 
         */
        void correctPh2LenToNominal();

    protected:
        
        BitType bitType;
        
        StuffBitType stuffBitType;
        
        BitValue bitValue;

        /**
         *
         */
        const BitTypeName bitTypeNames[29] =
        {
            {BIT_TYPE_SOF, "SOF"},
            {BIT_TYPE_BASE_ID, "Base identifer"},
            {BIT_TYPE_EXTENDED_ID, "Extended identifier"},
            {BIT_TYPE_RTR, "RTR"},
            {BIT_TYPE_IDE, "IDE"},
            {BIT_TYPE_SRR, "SRR"},
            {BIT_TYPE_EDL, "EDL"},
            {BIT_TYPE_R0, "R0 "},
            {BIT_TYPE_R1, "R1 "},
            {BIT_TYPE_BRS, "BRS"},
            {BIT_TYPE_ESI, "ESI"},
            {BIT_TYPE_DLC, "DLC"},
            {BIT_TYPE_DATA, "Data field"},
            {BIT_TYPE_STUFF_COUNT, "St.Ct."},
            {BIT_TYPE_STUFF_PARITY, "STP"},
            {BIT_TYPE_CRC, "CRC"},
            {BIT_TYPE_CRC_DELIMITER, "CRD"},
            {BIT_TYPE_ACK, "ACK"},
            {BIT_TYPE_ACK_DELIMITER, "ACD"},
            {BIT_TYPE_EOF, "End of Frame"},
            {BIT_TYPE_INTERMISSION, "Intermission"},
            {BIT_TYPE_IDLE, "Idle"},
            {BIT_TYPE_SUSPEND, "Suspend"},
            {BIT_TYPE_ACTIVE_ERROR_FLAG, "Active Error flag"},
            {BIT_TYPE_PASSIVE_ERROR_FLAG, "Passive Error flas"},
            {BIT_TYPE_ERROR_DELIMITER, "Error delimiter"},
            {BIT_TYPE_OVERLOAD_FLAG, "Overload flag"},
            {BIT_TYPE_OVERLOAD_DELIMITER, "Overload delimiter"}
        };

        // These hold information about bit timing and fact whether Bit-rate has
        // shifted so they are important for manipulation of bit cycles!
        BitTiming* nominalBitTiming;
        BitTiming* dataBitTiming;
        FrameFlags* frameFlags;

        /**
         * 
         */
        std::list<TimeQuanta> timeQuantas_;

        /**
         * 
         */
        void constructTimeQuantas(BitTiming *nominalBitTiming, BitTiming *dataBitTiming);

        BitPhase prevBitPhase(BitPhase bitPhase);
        BitPhase nextBitPhase(BitPhase bitPhase);

        BitRate getPhaseBitRate(BitPhase bitPhase);
        BitTiming* getPhaseBitTiming(BitPhase bitPhase);

        std::list<TimeQuanta>::iterator getFirstTimeQuantaIterator(BitPhase bitPhase);
        std::list<TimeQuanta>::iterator getLastTimeQuantaIterator(BitPhase bitPhase);
};

#endif