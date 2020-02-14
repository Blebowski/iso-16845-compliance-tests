/*
 * TODO: License
 */

#include <iostream>

#include "can.h"
#include "FrameFlags.h"

can::FrameFlags::FrameFlags()
{
    isFdf_ = CAN_2_0;
    isIde_ = BASE_IDENTIFIER;
    isEsi_ = ESI_ERROR_ACTIVE;
    isRtr_ = DATA_FRAME;
    isBrs_ = BIT_RATE_DONT_SHIFT;
}

can::FrameFlags::FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                             RemoteTransmissionRequest isRtr,
                             BitRateShift isBrs, ErrorStateIndicator isEsi)
{
    isFdf_ = isFdf;
    isIde_ = isIde;
    isRtr_ = isRtr;
    isBrs_ = isBrs;
    isEsi_ = isEsi;

    if (isFdf == CAN_FD && isRtr == RTR_FRAME){
        std::cerr << "Can't set RTR flag and FDF flag at once, RTR ignored!\n";
        isRtr = DATA_FRAME;
    }

    if (isFdf == CAN_2_0 && isBrs == BIT_RATE_SHIFT){
        std::cerr << "Can't set BRS flag when BRS flag is not set, BRS ignored!\n";
        isBrs = BIT_RATE_DONT_SHIFT;
    }

    if (isFdf == CAN_2_0 && isEsi == ESI_ERROR_PASSIVE){
        std::cerr << "Can't set ESI flag when FDF is not set, ESI ignored!\n";
        isEsi_ = ESI_ERROR_ACTIVE; // Error active is assumed default
    }
};