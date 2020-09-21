/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 25.8.2020
 * 
 *****************************************************************************/

#include <iostream>

#include "can.h"

using namespace can;

std::ostream& can::operator<<(std::ostream& os, FrameType &frame_type)
{
    if (frame_type == FrameType::Can2_0)
        os << "CAN 2.0";
    else
        os << "CAN FD";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, IdentifierType &identifier_type)
{
    if (identifier_type == IdentifierType::Base)
        os << "Base";
    else
        os << "Extended";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, BrsFlag &brs_flag)
{
    if (brs_flag == BrsFlag::Shift)
        os << "Shift";
    else
        os << "Don't shift";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, RtrFlag &rtr_flag)
{
    if (rtr_flag == RtrFlag::DataFrame)
        os << "Data frame";
    else
        os << "Remote frame";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, EsiFlag &esi_flag)
{
    if (esi_flag == EsiFlag::ErrorActive)
        os << "Error Active";
    else
        os << "Error Passive";
    return os;
}