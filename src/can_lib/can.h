#ifndef CAN_H
#define CAN_H
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

#include <iostream>

namespace can {

    enum class FrameKind
    {
        Can20,
        CanFd
    };

    std::ostream &operator<<(std::ostream &os, const FrameKind &frame_kind);

    enum class IdentKind
    {
        Base,
        Ext
    };

    std::ostream &operator<<(std::ostream &os, const IdentKind &ident_kind);

    enum class BrsFlag
    {
        DoShift,
        NoShift
    };

    std::ostream &operator<<(std::ostream &os, const BrsFlag &brs_flag);

    enum class RtrFlag
    {
        Data,
        Rtr
    };

    std::ostream &operator<<(std::ostream &os, const RtrFlag &rtr_flag);

    enum class EsiFlag
    {
        ErrAct,
        ErrPas
    };

    std::ostream &operator<<(std::ostream &os, const EsiFlag &esi_flag);

    enum class BitKind
    {
        Sof,
        BaseIdent,
        ExtIdent,
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
        StuffCnt,
        StuffParity,
        Crc,
        CrcDelim,
        Ack,
        AckDelim,
        Eof,
        Interm,
        Idle,
        SuspTrans,
        ActErrFlag,
        PasErrFlag,
        ErrDelim,
        OvrlFlag,
        OvrlDelim,
        Undefined
    };

    struct BitKindName
    {
        BitKind kind;
        std::string name;
    };

    enum class BitField
    {
        Sof,
        Arbit,
        Control,
        Data,
        Crc,
        Ack,
        Eof
    };

    enum class BitVal {
        Dominant = 0,
        Recessive = 1
    };

    enum class StuffKind
    {
        NoStuff,
        Normal,
        Fixed
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

    enum class FaultConfState
    {
        ErrAct,             // Error Active
        ErrPas,             // Error Passive
        BusOff,             // Bus-off
        Invalid
    };

    enum class SspType {
        Disabled,           // Secondary sample point disabled
        Offset,             // Offset only
        MeasAndOffset       // Measured value + offset
    };

    enum class CanVersion
    {
        Can20,              // CAN 2.0
        CanFdTol,           // CAN FD Tolerant
        CanFdEna            // CAN FD Enabled
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

    class Cycle;
    class TimeQuanta;

    class BitTiming;

    // Test related classes
    class DutInterface;
    class CtuCanFdInterface;

#define CAN_BASE_ID_MAX 2048
#define CAN_EXTENDED_ID_MAX 536870912
#define CAN_BASE_ID_ALL_ONES 0b11111111111

} // namespace can

#endif