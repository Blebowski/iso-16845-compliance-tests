/*
 * TODO: License
 */

#include <iostream>
#include "can.h"

#ifndef BIT
#define BIT

class can::Bit {

    public:
        Bit();
        Bit(BitType bitType, BitValue bitValue);
        Bit(BitType bitType, BitValue bitValue, StuffBitType stuffBitType);

        BitType bitType;
        StuffBitType stuffBitType;
        BitValue bitValue;

        bool setBitValue(BitValue bitValue);
        BitValue getbitValue();
        void flipBitValue();
        bool isStuffBit();
        bool isSingleBitField();
        std::string getStringValue();

        std::string getBitTypeName();

        BitValue getOppositeValue();

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
};

#endif