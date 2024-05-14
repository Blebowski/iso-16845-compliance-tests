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
#include <cmath>
#include <assert.h>

#include "can.h"
#include "Frame.h"
#include "FrameFlags.h"


can::Frame::Frame() :
    frm_flags_(FrameFlags()),
    randomize_dlc_(true),
    randomize_ident_(true),
    randomize_data_(true)
{}


can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc, int identifier, uint8_t *data) :
    frm_flags_(frame_flags)
{
    set_identifier(identifier);
    set_dlc(dlc);
    CopyData(data, data_len_);
}


can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc, int identifier) :
    frm_flags_(frame_flags),
    randomize_data_(true)
{
    set_identifier(identifier);
    set_dlc(dlc);
}

can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc) :
    frm_flags_(frame_flags),
    randomize_ident_(true),
    randomize_data_(true)
{
    set_dlc(dlc);
}


can::Frame::Frame(FrameFlags frame_flags) :
    frm_flags_(frame_flags),
    randomize_dlc_(true),
    randomize_ident_(true),
    randomize_data_(true)
{}


can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc, uint8_t *data) :
    frm_flags_(frame_flags),
    randomize_ident_(true)
{
    set_dlc(dlc);
    CopyData(data, data_len_);
}


void can::Frame::Randomize()
{
    /* First randomize flags, this gives constraints for further randomization */
    frm_flags_.Randomize();

    /*  Due to RTR Flag , Data length might have changed! Update it! */
    set_dlc(dlc_);

    if (randomize_ident_)
    {
        int max_ident_pow = 11;
        if (frame_flags().is_ide() == IdentKind::Ext)
            max_ident_pow = 29;
        set_identifier(rand() % ((int)pow(2, max_ident_pow)));
    }

    if (randomize_dlc_)
    {
        // Constrain here so that we get reasonable frames for CAN 2.0
        if (frame_flags().is_fdf() == FrameKind::CanFd)
            set_dlc(static_cast<uint8_t>(rand() % 0x9));
        else
            set_dlc(static_cast<uint8_t>(rand() % 0xF));
    }

    if (randomize_data_)
        for (int i = 0; i < data_len_; i++)
            data_[i] = static_cast<uint8_t>(rand() % 256);
}


bool can::Frame::operator==(const Frame rhs)
{
    if (identifier() != rhs.identifier())
        return false;
    if (dlc() != rhs.dlc())
        return false;
    if (frame_flags() != rhs.frame_flags())
        return false;
    if (data_length() != rhs.data_length())
        return false;

    for (int i = 0; i < data_length(); i++)
        if (data(i) != rhs.data(i))
            return false;

    return true;
}

bool can::Frame::operator!=(const can::Frame rhs)
{
    return !(*this == rhs);
}

void can::Frame::set_dlc(uint8_t dlc)
{
    assert(dlc < 17 && "Can't set DLC higher than 16");

    dlc_ = dlc;
    data_len_ = ConvDlcToDataLen(dlc);
}

void can::Frame::set_data_length(int dataLenght)
{
    assert(IsValidDataLen(dataLenght) && "Invalid data length");

    assert(!(frm_flags_.is_fdf() == FrameKind::Can20 && dataLenght > 8) &&
            "Can't set data length to more than 8 on CAN 2.0 frame");

    data_len_ = dataLenght;
    dlc_ = ConvDataLenToDlc(dataLenght);
}

void can::Frame::set_identifier(int identifier)
{
    assert((!(frm_flags_.is_ide() == IdentKind::Base && identifier >= pow(2.0, 11)),
             "Can't set Base identifier larger than 2^11"));

    assert((!(identifier >= pow(2.0, 29)), "Can't set Extended identifier larger than 2^29"));

    ident_ = identifier;
}

void can::Frame::CopyData(uint8_t *data, int dataLen)
{
    if (data == 0){
        std::cerr << "Null pointer: source data" << std::endl;
        return;
    }

    for (int i = 0; i < dataLen; i++)
        data_[i] = data[i];
}

int can::Frame::ConvDlcToDataLen(uint8_t dlc)
{
    if (frm_flags_.is_fdf() == FrameKind::Can20 && frm_flags_.is_rtr() == RtrFlag::Rtr)
        return 0;

    if (frm_flags_.is_fdf() == FrameKind::Can20 && dlc >= 0x8)
        return 0x8;

    assert(dlc <= 16);

    return dlc_to_data_length_table_[dlc][1];
}

uint8_t can::Frame::ConvDataLenToDlc(int dataLenght)
{
    for (int i = 0; i < 16; i++){
        if (uint8_t(dlc_to_data_length_table_[i][1]) == dataLenght)
            return static_cast<uint8_t>(dlc_to_data_length_table_[i][0]);
    }
    return -1;
}

bool can::Frame::IsValidDataLen(int dataLenght)
{
    for (int i = 0; i < 16; i++)
        if (dlc_to_data_length_table_[i][1] == dataLenght)
            return true;
    return false;
}

void can::Frame::Print()
{
    std::cout << std::string(80, '*') << '\n';
    std::cout << "CAN Frame:" << '\n';
    std::cout << "FDF: " << frm_flags_.is_fdf() << '\n';
    std::cout << "IDE: " << frm_flags_.is_ide() << '\n';

    if (frm_flags_.is_fdf() == FrameKind::CanFd)
        std::cout << "BRS: " << frm_flags_.is_brs() << '\n';
    else
        std::cout << "RTR: " << frm_flags_.is_rtr() << '\n';

    std::cout << "DLC: 0x" << std::hex << +dlc_ << '\n';
    std::cout << "ESI: " << frm_flags_.is_esi() << '\n';
    std::cout << "Data field length: " << data_len_ << '\n';
    std::cout << "Identifier: " << std::hex << ident_ << '\n';

    std::cout << "Data: ";
    for (int i = 0; i < data_len_; i++)
        std::cout << "0x" << std::hex << +data_[i] << " ";
    std::cout << '\n';;

    std::cout << std::string(80, '*') << std::endl;
}