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
    SetDefaultValues();
    RandomizeEnableAll();
}

can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide,
                             RtrFlag is_rtr,
                             BrsFlag is_brs, EsiFlag is_esi)
{
    is_fdf_ = is_fdf;
    is_ide_ = is_ide;
    is_rtr_ = is_rtr;
    is_brs_ = is_brs;
    is_esi_ = is_esi;

    CorrectFlags();
    RandomizeDisableAll();
};

can::FrameFlags::FrameFlags(FrameType is_fdf, RtrFlag is_rtr, BrsFlag is_brs, EsiFlag is_esi)
{
    SetDefaultValues();

    is_fdf_ = is_fdf;
    is_rtr_ = is_rtr;
    is_brs_ = is_brs;
    is_esi_ = is_esi;

    RandomizeDisableAll();
    randomize_ide = true;
};

can::FrameFlags::FrameFlags(FrameType is_fdf, RtrFlag is_rtr, EsiFlag is_esi)
{
    SetDefaultValues();

    is_fdf_ = is_fdf;
    is_rtr_ = is_rtr;
    is_esi_ = is_esi;

    RandomizeDisableAll();
    randomize_ide = true;
    randomize_brs = true;
};

can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide,
                            RtrFlag is_rtr)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_ide_ = is_ide;
    is_rtr_ = is_rtr;

    CorrectFlags();
    RandomizeDisableAll();
    randomize_esi = true;
    randomize_brs = true;
}


can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_ide_ = is_ide;

    RandomizeEnableAll();
    randomize_fdf = false;
    randomize_ide = false;
}


can::FrameFlags::FrameFlags(FrameType is_fdf, RtrFlag is_rtr)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_rtr_ = is_rtr;

    CorrectFlags();
    RandomizeEnableAll();
    randomize_fdf = false;
    randomize_rtr = false;
}

can::FrameFlags::FrameFlags(FrameType is_fdf)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;

    RandomizeEnableAll();
    randomize_fdf = false;
}


can::FrameFlags::FrameFlags(IdentifierType is_ide)
{
    SetDefaultValues();
    
    is_ide_ = is_ide;

    RandomizeEnableAll();
    randomize_ide = false;
}


can::FrameFlags::FrameFlags(FrameType is_fdf, BrsFlag is_brs)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_brs_ = is_brs;

    CorrectFlags();
    RandomizeEnableAll();
    randomize_fdf = false;
    randomize_brs = false;
}


can::FrameFlags::FrameFlags(FrameType is_fdf, BrsFlag is_brs,
                            EsiFlag is_esi)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_brs_ = is_brs;
    is_esi_ = is_esi;

    CorrectFlags();
    RandomizeEnableAll();
    randomize_fdf = false;
    randomize_brs = false;
    randomize_esi = false;
}


can::FrameFlags::FrameFlags(FrameType is_fdf, EsiFlag is_esi)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_esi_ = is_esi;

    CorrectFlags();
    RandomizeEnableAll();
    randomize_fdf = false;
    randomize_esi = false;
}


can::FrameFlags::FrameFlags(FrameType is_fdf, IdentifierType is_ide,
                            EsiFlag is_esi)
{
    SetDefaultValues();
    
    is_fdf_ = is_fdf;
    is_esi_ = is_esi;
    is_ide_ = is_ide;

    CorrectFlags();
    RandomizeEnableAll();
    randomize_fdf = false;
    randomize_esi = false;
    randomize_ide = false;
}

bool operator==(const can::FrameFlags &lhs, const can::FrameFlags rhs)
{
    if (lhs.is_brs_ != rhs.is_brs_)
        return false;
    if (lhs.is_esi_ != rhs.is_esi_)
        return false;
    if (lhs.is_fdf_ != rhs.is_fdf_)
        return false;
    if (lhs.is_ide_ != rhs.is_ide_)
        return false;
    if (lhs.is_rtr_ != rhs.is_rtr_)
        return false;
    return true;
}


void can::FrameFlags::Randomize()
{
    if (randomize_fdf)
    {
        if (rand() % 2 == 1)
            is_fdf_ = FrameType::Can2_0;
        else
            is_fdf_ = FrameType::CanFd;
    }

    if (randomize_ide)
    {
        if (rand() % 2 == 1)
            is_ide_ = IdentifierType::Base;
        else
            is_ide_ = IdentifierType::Extended;
    }    

    if (randomize_rtr)
    {
        if (is_fdf_ == FrameType::CanFd)
            is_rtr_ = RtrFlag::DataFrame;
        else if (rand() % 4 == 1)
            is_rtr_ = RtrFlag::RtrFrame;
        else
            is_rtr_ = RtrFlag::DataFrame;
    }

    if (randomize_brs)
    {
        if (is_fdf_ == FrameType::Can2_0)
            is_brs_ = BrsFlag::DontShift;
        else if (rand() % 2 == 1)
            is_brs_ = BrsFlag::Shift;
        else
            is_brs_ = BrsFlag::DontShift;
    }

    if (randomize_esi)
    {
        if (is_fdf_ == FrameType::Can2_0)
            is_esi_ = EsiFlag::ErrorActive;
        else if (rand() % 2 == 1)
            is_esi_ = EsiFlag::ErrorPassive;
        else
            is_esi_ = EsiFlag::ErrorActive;
    }
}


void can::FrameFlags::RandomizeEnableAll()
{
    randomize_fdf = true;
    randomize_ide = true;
    randomize_rtr = true;
    randomize_brs = true;
    randomize_esi = true;
}


void can::FrameFlags::RandomizeDisableAll()
{
    randomize_fdf = false;
    randomize_ide = false;
    randomize_rtr = false;
    randomize_brs = false;
    randomize_esi = false;
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

void can::FrameFlags::SetDefaultValues()
{
    is_fdf_ = FrameType::Can2_0;
    is_ide_ = IdentifierType::Base;
    is_esi_ = EsiFlag::ErrorActive;
    is_rtr_ = RtrFlag::DataFrame;
    is_brs_ = BrsFlag::DontShift;
}