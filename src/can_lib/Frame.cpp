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
#include <cmath>
#include <assert.h>

#include "can.h"
#include "Frame.h"
#include "FrameFlags.h"


///////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////

can::Frame::Frame() :
    frame_flags_(FrameFlags()),
    randomize_dlc_(true),
    randomize_identifier_(true),
    randomize_data_(true)
{}

can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc, int identifier, uint8_t *data) :
    frame_flags_(frame_flags)
{
    set_identifier(identifier);
    set_dlc(dlc);
    CopyData(data, data_lenght_);
}

can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc, int identifier) :
    frame_flags_(frame_flags),
    randomize_data_(true)
{
    set_identifier(identifier);
    set_dlc(dlc);
}

can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc) :
    frame_flags_(frame_flags),
    randomize_identifier_(true),
    randomize_data_(true)
{
    set_dlc(dlc);
}

can::Frame::Frame(FrameFlags frame_flags) :
    frame_flags_(frame_flags),
    randomize_dlc_(true),
    randomize_identifier_(true),
    randomize_data_(true)
{}

can::Frame::Frame(FrameFlags frame_flags, uint8_t dlc, uint8_t *data) :
    frame_flags_(frame_flags),
    randomize_identifier_(true)
{
    set_dlc(dlc);
    CopyData(data, data_lenght_);
}

void can::Frame::Randomize()
{
    /* First randomize flags, this gives constraints for further randomization */
    frame_flags_.Randomize();

    /*  Due to RTR Flag , Data length might have changed! Update it! */
    set_dlc(dlc_);

    if (randomize_identifier_)
    {
        int max_ident_pow = 11;
        if (frame_flags().is_ide() == IdentifierType::Extended)
            max_ident_pow = 29;
        set_identifier(rand() % ((int)pow(2, max_ident_pow)));
    }

    if (randomize_dlc_)
    {
        // Constrain here so that we get reasonable frames for CAN 2.0
        if (frame_flags().is_fdf() == FrameType::CanFd)
            set_dlc(rand() % 0x9);
        else
            set_dlc(rand() % 0xF);
    }

    if (randomize_data_)
        for (int i = 0; i < 64; i++)
            data_[i] = rand() % 256;
}


///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

can::FrameFlags can::Frame::frame_flags()
{
    return frame_flags_;
}

uint8_t can::Frame::dlc()
{
    return dlc_;
}

int can::Frame::data_length()
{
    return data_lenght_;
}

int can::Frame::identifier()
{
    return identifier_;
}

uint8_t* can::Frame::data()
{
    return data_;
}

uint8_t can::Frame::data(int index)
{
    return data_[index];
}

///////////////////////////////////////////////////////////////////////////////
// Setters
///////////////////////////////////////////////////////////////////////////////

void can::Frame::set_dlc(uint8_t dlc)
{
    assert(dlc < 17 && "Can't set DLC higher than 16");
    
    dlc_ = dlc;
    data_lenght_ = ConvertDlcToDataLenght(dlc);
}

void can::Frame::set_data_lenght(int dataLenght)
{
    assert(IsValidDataLength(dataLenght) && "Invalid data length");

    assert(!(frame_flags_.is_fdf() == FrameType::Can2_0 && dataLenght > 8) &&
            "Can't set data length to more than 8 on CAN 2.0 frame");
    
    data_lenght_ = dataLenght;
    dlc_ = ConvertDataLenghtToDlc(dataLenght);
}

void can::Frame::set_identifier(int identifier)
{
    assert((!(frame_flags_.is_ide() == IdentifierType::Base && identifier >= pow(2.0, 11)),
             "Can't set Base identifier larger than 2^11"));

    assert((!(identifier >= pow(2.0, 29)), "Can't set Extended identifier larger than 2^29"));

    identifier_ = identifier;
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

int can::Frame::ConvertDlcToDataLenght(uint8_t dlc)
{
    if (frame_flags_.is_fdf() == FrameType::Can2_0 && frame_flags_.is_rtr() == RtrFlag::RtrFrame)
        return 0;

    if (frame_flags_.is_fdf() == FrameType::Can2_0 && dlc >= 0x8)
        return 0x8;

    assert(dlc <= 16);

    return dlc_to_data_lenght_table_[dlc][1];
}

uint8_t can::Frame::ConvertDataLenghtToDlc(int dataLenght)
{
    for (int i = 0; i < 16; i++){
        if (uint8_t(dlc_to_data_lenght_table_[i][1]) == dataLenght)
            return dlc_to_data_lenght_table_[i][0];
    }
    return -1;
}

bool can::Frame::IsValidDataLength(int dataLenght)
{
    for (int i = 0; i < 16; i++)
        if (dlc_to_data_lenght_table_[i][1] == dataLenght)
            return true;
    return false;
}

void can::Frame::Print()
{
    std::cout << std::string(80, '*') << '\n';
    std::cout << "CAN Frame:" << '\n';
    std::cout << "FDF: " << frame_flags_.is_fdf() << '\n';
    std::cout << "IDE: " << frame_flags_.is_ide() << '\n';

    if (frame_flags_.is_fdf() == FrameType::CanFd)
        std::cout << "BRS: " << frame_flags_.is_brs() << '\n';
    else
        std::cout << "RTR: " << frame_flags_.is_rtr() << '\n';
    
    std::cout << "DLC: 0x" << std::hex << +dlc_ << '\n';
    std::cout << "ESI: " << frame_flags_.is_esi() << '\n';
    std::cout << "Data field length: " << data_lenght_ << '\n';
    std::cout << "Identifier: " << std::hex << identifier_ << '\n';

    std::cout << "Data: ";
    for (int i = 0; i < data_lenght_; i++)
        std::cout << "0x" << std::hex << +data_[i] << " ";
    std::cout << '\n';;

    std::cout << std::string(80, '*') << std::endl;
}