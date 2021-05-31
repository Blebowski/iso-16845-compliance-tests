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


can::FrameFlags::FrameFlags() :
    randomize_fdf_(true),
    randomize_ide_(true),
    randomize_rtr_(true),
    randomize_brs_(true),
    randomize_esi_(true)
{}

can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide, RtrFlag is_rtr,
                             BrsFlag is_brs, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_ide_(is_ide),
                            is_rtr_(is_rtr),
                            is_brs_(is_brs),
                            is_esi_(is_esi)                           
{
    CorrectFlags();
}

can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide, RtrFlag is_rtr, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_ide_(is_ide),
                            is_rtr_(is_rtr),
                            is_esi_(is_esi),
                            randomize_brs_(true)
{
    CorrectFlags();
}

can::FrameFlags::FrameFlags(FrameType is_fdf, RtrFlag is_rtr, BrsFlag is_brs, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_rtr_(is_rtr),
                            is_brs_(is_brs),
                            is_esi_(is_esi),
                            randomize_ide_(true)
{
    CorrectFlags();
}

can::FrameFlags::FrameFlags(FrameType is_fdf, RtrFlag is_rtr, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_rtr_(is_rtr),
                            is_esi_(is_esi),
                            randomize_ide_(true),
                            randomize_brs_(true)
{
    CorrectFlags();
}

can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide, RtrFlag is_rtr):
                            is_fdf_(is_fdf),
                            is_ide_(is_ide),
                            is_rtr_(is_rtr),
                            randomize_brs_(true),
                            randomize_esi_(true)
                            
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide):
                            is_fdf_(is_fdf),
                            is_ide_(is_ide),
                            randomize_rtr_(true),
                            randomize_brs_(true),
                            randomize_esi_(true)
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(FrameType is_fdf, RtrFlag is_rtr):
                            is_fdf_(is_fdf),
                            is_rtr_(is_rtr),
                            randomize_ide_(true),
                            randomize_brs_(true),
                            randomize_esi_(true)
{
    CorrectFlags();
}

can::FrameFlags::FrameFlags(FrameType is_fdf):
                            is_fdf_(is_fdf),
                            randomize_ide_(true),
                            randomize_rtr_(true),
                            randomize_brs_(true),
                            randomize_esi_(true)
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(IdentifierType is_ide):
                            is_ide_(is_ide),
                            randomize_fdf_(true),
                            randomize_rtr_(true),
                            randomize_brs_(true),
                            randomize_esi_(true)
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(FrameType is_fdf, BrsFlag is_brs):
                            is_fdf_(is_fdf),
                            is_brs_(is_brs),
                            randomize_ide_(true),
                            randomize_rtr_(true),
                            randomize_esi_(true)
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(FrameType is_fdf, BrsFlag is_brs, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_brs_(is_brs),
                            is_esi_(is_esi),
                            randomize_ide_(true),
                            randomize_rtr_(true)
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(FrameType is_fdf, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_esi_(is_esi),
                            randomize_ide_(true),
                            randomize_rtr_(true),
                            randomize_brs_(true)
{
    CorrectFlags();
}


can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide, EsiFlag is_esi):
                            is_fdf_(is_fdf),
                            is_ide_(is_ide),
                            is_esi_(is_esi),
                            randomize_rtr_(true),
                            randomize_brs_(true)
{
    CorrectFlags();
}

bool can::FrameFlags::operator==(const can::FrameFlags rhs)
{
    if (is_brs_ != rhs.is_brs_)
        return false;
    if (is_esi_ != rhs.is_esi_)
        return false;
    if (is_fdf_ != rhs.is_fdf_)
        return false;
    if (is_ide_ != rhs.is_ide_)
        return false;
    if (is_rtr_ != rhs.is_rtr_)
        return false;
    return true;
}

bool can::FrameFlags::operator!=(const FrameFlags rhs)
{
    return !(*this==rhs);
}

void can::FrameFlags::Randomize()
{
    if (randomize_fdf_)
    {
        if (rand() % 2 == 1)
            is_fdf_ = FrameType::Can2_0;
        else
            is_fdf_ = FrameType::CanFd;
    }

    if (randomize_ide_)
    {
        if (rand() % 2 == 1)
            is_ide_ = IdentifierType::Base;
        else
            is_ide_ = IdentifierType::Extended;
    }    

    if (randomize_rtr_)
    {
        if (is_fdf_ == FrameType::CanFd)
            is_rtr_ = RtrFlag::DataFrame;
        else if (rand() % 4 == 1)
            is_rtr_ = RtrFlag::RtrFrame;
        else
            is_rtr_ = RtrFlag::DataFrame;
    }

    if (randomize_brs_)
    {
        if (is_fdf_ == FrameType::Can2_0)
            is_brs_ = BrsFlag::DontShift;
        else if (rand() % 2 == 1)
            is_brs_ = BrsFlag::Shift;
        else
            is_brs_ = BrsFlag::DontShift;
    }

    if (randomize_esi_)
    {
        if (is_fdf_ == FrameType::Can2_0)
            is_esi_ = EsiFlag::ErrorActive;
        else if (rand() % 2 == 1)
            is_esi_ = EsiFlag::ErrorPassive;
        else
            is_esi_ = EsiFlag::ErrorActive;
    }
}


void can::FrameFlags::CorrectFlags()
{
    if (is_fdf_ == FrameType::CanFd && is_rtr_ == RtrFlag::RtrFrame){
        std::cerr << "Can't set RTR flag and FDF flag at once, RTR ignored!\n";
        is_rtr_ = RtrFlag::DataFrame;
    }

    if (is_fdf_ == FrameType::Can2_0 && is_brs_ == BrsFlag::Shift){
        std::cerr << "Can't set BRS flag when FDF flag is not set, BRS ignored!\n";
        is_brs_ = BrsFlag::DontShift;
    }

    if (is_fdf_ == FrameType::Can2_0 && is_esi_ == EsiFlag::ErrorPassive){
        std::cerr << "Can't set ESI flag when FDF is not set, ESI ignored!\n";
        is_esi_ = EsiFlag::ErrorActive;
    }
}