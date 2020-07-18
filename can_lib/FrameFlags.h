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

#include "can.h"

#ifndef FRAME_FLAGS
#define FRAME_FLAGS

class can::FrameFlags
{
    public:

        // CAN frame flags
        FlexibleDataRate isFdf_;
        ExtendedIdentifier isIde_;
        RemoteTransmissionRequest isRtr_;
        BitRateShift isBrs_;
        ErrorStateIndicator isEsi_;

        // Randomization attributes
        bool randomizeFdf;
        bool randomizeIde;
        bool randomizeRtr;
        bool randomizeBrs;
        bool randomizeEsi;

        FrameFlags();
        FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                   RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                   ErrorStateIndicator isEsi);
        FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                   RemoteTransmissionRequest isRtr);
        FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde);
        FrameFlags(FlexibleDataRate isFdf, RemoteTransmissionRequest isRtr);
        FrameFlags(FlexibleDataRate isFdf);
        FrameFlags(ExtendedIdentifier isIde);
        FrameFlags(FlexibleDataRate isFdf, BitRateShift isBrs);
        FrameFlags(FlexibleDataRate isFdf, BitRateShift isBrs, ErrorStateIndicator isEsi);
        FrameFlags(FlexibleDataRate isFdf, ErrorStateIndicator isEsi);

        void randomize();
        void randomizeEnableAll();
        void randomizeDisableAll();

        friend bool operator==(const FrameFlags &lhs, const FrameFlags rhs);
};

#endif