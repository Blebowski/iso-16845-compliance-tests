/*
 * TODO: License
 */

#include "can.h"

#ifndef FRAME_FLAGS
#define FRAME_FLAGS

class can::FrameFlags
{
    public:
        FlexibleDataRate isFdf_;
        ExtendedIdentifier isIde_;
        RemoteTransmissionRequest isRtr_;
        BitRateShift isBrs_;
        ErrorStateIndicator isEsi_;

        FrameFlags();
        FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                   RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                   ErrorStateIndicator isEsi);
};

#endif