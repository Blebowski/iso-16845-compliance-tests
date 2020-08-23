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

    enum class FrameType
    {
        Can2_0,
        CanFd
    };

    std::ostream &operator<<(std::ostream &os, FrameType &frame_type);

    enum class IdentifierType
    {
        Base,
        Extended
    };

    std::ostream &operator<<(std::ostream &os, IdentifierType &identifier_type);

    enum class BrsFlag
    {
        Shift,
        DontShift
    };

    std::ostream &operator<<(std::ostream &os, BrsFlag &brs_flag);

    enum class RtrFlag
    {
        DataFrame,
        RtrFrame
    };

    std::ostream &operator<<(std::ostream &os, RtrFlag &rtr_flag);
    
    enum class EsiFlag
    {
        ErrorActive,
        ErrorPassive
    };

    std::ostream &operator<<(std::ostream &os, EsiFlag &esi_flag);

    enum class BitType
    {
        Sof,
        BaseIdentifier,
        IdentifierExtension,
        Rtr,
        Ide,
        Srr,
        Edl,
        R0,
        R1,
        Brs,
        Esi,
        Dlc,
        Data,
        StuffCount,
        StuffParity,
        Crc,
        CrcDelimiter,
        Ack,
        AckDelimiter,
        Eof,
        Intermission,
        Idle,
        Suspend,
        ActiveErrorFlag,
        PassiveErrorFlag,
        ErrorDelimiter,
        OverloadFlag,
        OverloadDelimiter
    };

    struct BitTypeName
    {
        BitType bit_type;
        std::string name;
    };

    enum class BitValue {
        Dominant = 0,
        Recessive = 1
    };

    enum class StuffBitType
    {
        NoStuffBit,
        NormalStuffBit,
        FixedStuffBit
    };

    enum class BitRate
    {
        Nominal,
        Data
    };

    enum class BitPhase
    {
        Sync,
        Prop,
        Ph1,
        Ph2
    };

    enum class FaultConfinementState
    {
        ErrorActive,
        ErrorPassive,
        BusOff
    };

    enum class SspType {
        Disabled,           // Secondary sample point disabled
        Offset,             // Offset only
        MeasuredPlusOffset  // Measured value + offset
    };

    enum class CanVersion
    {
        Can_2_0,
        CanFdTolerant,
        CanFdEnabled
    };

    /* Classes modeling CAN frame:
     *   Frame          - contains metadata (DLC, ID, Data, flags) of CAN frame
     *   Bit            - Represents single bit on CAN bus. Contains time quantas.
     *   TimeQuanta     - Represents single Time Quanta. Contains list of Cycle bit values.
     *   CycleBitValue  - Value of bit during single cycle.
     *   FrameFlags     - Frame flags (RTR, IDE, BRS, etc...)
     *   BitTiming      - Timing parameters of CAN bus.
     *   BitFrame       - CAN frame with representation of each bit and its value.
     */

    class Bit;
    class Frame;
    class BitFrame;

    class FrameFlags;

    class CycleBitValue;
    class TimeQuanta;

    class BitTiming;

    // Test related classes
    class DutInterface;
    class CtuCanFdInterface;

}; // namespace can

#endif