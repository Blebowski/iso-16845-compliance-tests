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

#include "can.h"
#include "FrameFlags.h"

can::FrameFlags::FrameFlags()
{
    // These are considered default values.
    isFdf_ = CAN_2_0;
    isIde_ = BASE_IDENTIFIER;
    isEsi_ = ESI_ERROR_ACTIVE;
    isRtr_ = DATA_FRAME;
    isBrs_ = BIT_RATE_DONT_SHIFT;

    randomizeEnableAll();
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

    randomizeDisableAll();
};


can::FrameFlags::FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                            RemoteTransmissionRequest isRtr)
{
    isFdf_ = isFdf;
    isIde_ = isIde;
    isRtr_ = isRtr;

    if (isFdf == CAN_FD && isRtr == RTR_FRAME){
        std::cerr << "Can't set RTR flag and FDF flag at once, RTR ignored!\n";
        isRtr = DATA_FRAME;
    }

    randomizeDisableAll();
    randomizeEsi = true;
    randomizeBrs = true;
}


can::FrameFlags::FrameFlags(FlexibleDataRate isFdf, ExtendedIdentifier isIde)
{
    isFdf_ = isFdf;
    isIde_ = isIde;

    randomizeEnableAll();
    randomizeFdf = false;
    randomizeIde = false;
}


can::FrameFlags::FrameFlags(FlexibleDataRate isFdf, RemoteTransmissionRequest isRtr)
{
    isFdf_ = isFdf;
    isRtr_ = isRtr;

    randomizeEnableAll();
    randomizeFdf = false;
    randomizeRtr = false;
}

can::FrameFlags::FrameFlags(FlexibleDataRate isFdf)
{
    isFdf_ = isFdf;

    randomizeEnableAll();
    randomizeFdf = false;
}


can::FrameFlags::FrameFlags(ExtendedIdentifier isIde)
{
    isIde_ = isIde;

    randomizeEnableAll();
    randomizeIde = false;
}


can::FrameFlags::FrameFlags(FlexibleDataRate isFdf, BitRateShift isBrs)
{
    isFdf_ = isFdf;
    isBrs_ = isBrs;

    randomizeEnableAll();
    randomizeFdf = false;
    randomizeBrs = false;
}


can::FrameFlags::FrameFlags(FlexibleDataRate isFdf, BitRateShift isBrs,
                            ErrorStateIndicator isEsi)
{
    isFdf_ = isFdf;
    isBrs_ = isBrs;
    isEsi_ = isEsi;

    randomizeEnableAll();
    randomizeFdf = false;
    randomizeBrs = false;
    randomizeEsi = false;
}

bool operator==(const can::FrameFlags &lhs, const can::FrameFlags rhs)
{
    if (lhs.isBrs_ != rhs.isBrs_)
        return false;
    if (lhs.isEsi_ != rhs.isEsi_)
        return false;
    if (lhs.isFdf_ != rhs.isFdf_)
        return false;
    if (lhs.isIde_ != rhs.isIde_)
        return false;
    if (lhs.isRtr_ != rhs.isRtr_)
        return false;
    return true;
}


void can::FrameFlags::randomize()
{
    if (randomizeFdf)
    {
        if (rand() % 2 == 1)
            isFdf_ = FlexibleDataRate::CAN_2_0;
        else
            isFdf_ = FlexibleDataRate::CAN_FD;
    }

    if (randomizeIde)
    {
        if (rand() % 2 == 1)
            isIde_ = ExtendedIdentifier::BASE_IDENTIFIER;
        else
            isIde_ = ExtendedIdentifier::EXTENDED_IDENTIFIER;
    }    

    if (randomizeRtr)
    {
        if (isFdf_ == FlexibleDataRate::CAN_FD)
            isRtr_ = RemoteTransmissionRequest::DATA_FRAME;
        else if (rand() % 4 == 1)
            isRtr_ = RemoteTransmissionRequest::RTR_FRAME;
        else
            isRtr_ = RemoteTransmissionRequest::DATA_FRAME;
    }

    if (randomizeBrs)
    {
        if (isFdf_ == FlexibleDataRate::CAN_2_0)
            isBrs_ = BitRateShift::BIT_RATE_DONT_SHIFT;
        else if (rand() % 2 == 1)
            isBrs_ = BitRateShift::BIT_RATE_SHIFT;
        else
            isBrs_ = BitRateShift::BIT_RATE_DONT_SHIFT;
    }

    if (randomizeEsi)
    {
        if (isFdf_ == FlexibleDataRate::CAN_2_0)
            isEsi_ = ErrorStateIndicator::ESI_ERROR_ACTIVE;
        else if (rand() % 2 == 1)
            isEsi_ = ErrorStateIndicator::ESI_ERROR_PASSIVE;
        else
            isEsi_ = ErrorStateIndicator::ESI_ERROR_ACTIVE;
    }
}


void can::FrameFlags::randomizeEnableAll()
{
    randomizeFdf = true;
    randomizeIde = true;
    randomizeRtr = true;
    randomizeBrs = true;
    randomizeEsi = true;
}


void can::FrameFlags::randomizeDisableAll()
{
    randomizeFdf = false;
    randomizeIde = false;
    randomizeRtr = false;
    randomizeBrs = false;
    randomizeEsi = false;
}