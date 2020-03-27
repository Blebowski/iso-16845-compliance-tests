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