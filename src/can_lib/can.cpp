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
 * @date 25.8.2020
 *
 *****************************************************************************/

#include <iostream>

#include "can.h"

using namespace can;

std::ostream& can::operator<<(std::ostream& os, const FrameKind &frame_kind)
{
    if (frame_kind == FrameKind::Can20)
        os << "CAN 2.0";
    else
        os << "CAN FD";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, const IdentKind &ident_kind)
{
    if (ident_kind == IdentKind::Base)
        os << "Base";
    else
        os << "Extended";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, const BrsFlag &brs_flag)
{
    if (brs_flag == BrsFlag::DoShift)
        os << "Shift";
    else
        os << "Don't shift";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, const RtrFlag &rtr_flag)
{
    if (rtr_flag == RtrFlag::Data)
        os << "Data frame";
    else
        os << "Remote frame";
    return os;
}


std::ostream& can::operator<<(std::ostream& os, const EsiFlag &esi_flag)
{
    if (esi_flag == EsiFlag::ErrAct)
        os << "Error Active";
    else
        os << "Error Passive";
    return os;
}