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

#ifndef CAN
#define CAN

#include <iostream>

namespace can {

    enum class FrameType
    {
        Can2_0,
        CanFd
    };

    std::ostream &operator<<(std::ostream &os, const FrameType &frame_type);

    enum class IdentifierType
    {
        Base,
        Extended
    };

    std::ostream &operator<<(std::ostream &os, const IdentifierType &identifier_type);

    enum class BrsFlag
    {
        Shift,
        DontShift
    };

    std::ostream &operator<<(std::ostream &os, const BrsFlag &brs_flag);

    enum class RtrFlag
    {
        DataFrame,
        RtrFrame
    };

    std::ostream &operator<<(std::ostream &os, const RtrFlag &rtr_flag);
    
    enum class EsiFlag
    {
        ErrorActive,
        ErrorPassive
    };

    std::ostream &operator<<(std::ostream &os, const EsiFlag &esi_flag);

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

    enum class BitField
    {
        Sof,
        Arbitration,
        Control,
        Data,
        Crc,
        Ack,
        Eof
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
        BusOff,
        Invalid
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

} // namespace can

#endif