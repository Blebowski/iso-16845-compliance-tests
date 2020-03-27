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

#ifndef CAN
#define CAN

#include <iostream>

namespace can {

    enum FlexibleDataRate
    {
        CAN_2_0,
        CAN_FD
    };

    enum ExtendedIdentifier
    {
        BASE_IDENTIFIER,
        EXTENDED_IDENTIFIER
    };

    enum BitRateShift
    {
        BIT_RATE_SHIFT,
        BIT_RATE_DONT_SHIFT
    };

    enum RemoteTransmissionRequest
    {
        RTR_FRAME,
        DATA_FRAME
    };

    enum ErrorStateIndicator
    {
        ESI_ERROR_ACTIVE,
        ESI_ERROR_PASSIVE
    };

    enum BitType
    {
        BIT_TYPE_SOF,
        BIT_TYPE_BASE_ID,
        BIT_TYPE_EXTENDED_ID,
        BIT_TYPE_RTR,
        BIT_TYPE_IDE,
        BIT_TYPE_SRR,
        BIT_TYPE_EDL,
        BIT_TYPE_R0,
        BIT_TYPE_R1,
        BIT_TYPE_BRS,
        BIT_TYPE_ESI,
        BIT_TYPE_DLC,
        BIT_TYPE_DATA,
        BIT_TYPE_STUFF_COUNT,
        BIT_TYPE_STUFF_PARITY,
        BIT_TYPE_CRC,
        BIT_TYPE_CRC_DELIMITER,
        BIT_TYPE_ACK,
        BIT_TYPE_ACK_DELIMITER,
        BIT_TYPE_EOF,
        BIT_TYPE_INTERMISSION,
        BIT_TYPE_IDLE,
        BIT_TYPE_SUSPEND,
        BIT_TYPE_ACTIVE_ERROR_FLAG,
        BIT_TYPE_PASSIVE_ERROR_FLAG,
        BIT_TYPE_ERROR_DELIMITER,
        BIT_TYPE_OVERLOAD_FLAG,
        BIT_TYPE_OVERLOAD_DELIMITER
    };

    struct BitTypeName
    {
        BitType bitType;
        std::string name;
    };

    enum BitValue {
        DOMINANT = 0,
        RECESSIVE = 1
    };

    enum StuffBitType
    {
        STUFF_NO,
        STUFF_NORMAL,
        STUFF_FIXED
    };

    enum BitRate
    {
        NOMINAL_BIT_RATE,
        DATA_BIT_RATE
    };

    enum BitPhase
    {
        SYNC_PHASE,
        PROP_PHASE,
        PH1_PHASE,
        PH2_PHASE
    };

    enum ErrorState
    {
        ERROR_ACTIVE,
        ERROR_PASSIVE,
        BUS_OFF
    };

    enum CanVersion
    {
        CAN_2_0_VERSION,
        CAN_FD_TOLERANT_VERSION,
        CAN_FD_ENABLED_VERSION
    };

    class Bit;
    class CycleBit;
    
    class Frame;
    class BitFrame;
    class CycleFrame;

    class FrameFlags;

    class CycleBitValue;
    class TimeQuanta;

    class BitTiming;

    // Test related classes
    class DutInterface;
    class CtuCanFdInterface;

}; // namespace can

#endif