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

#include "Frame.h"
#include "FrameFlags.h"


///////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////

can::Frame::Frame()
{
    frame_flags_ = FrameFlags();
    dlc_ = 0;
    identifier_ = 0;
    data_lenght_ = 0;

    randomize_dlc = true;
    randomize_identifier = true;
    randomize_data = true;
}

can::Frame::Frame(FrameFlags frameFlags, uint8_t dlc, int identifier,
                  uint8_t *data)
{
    frame_flags_ = frameFlags;   
    set_identifer(identifier);
    set_dlc(dlc);
    CopyData(data, data_lenght_);

    randomize_dlc = false;
    randomize_identifier = false;
    randomize_data = false;
}


can::Frame::Frame(FrameFlags frameFlags, uint8_t dlc, int identifier)
{
    frame_flags_ = frameFlags;
    set_identifer(identifier);
    set_dlc(dlc);

    randomize_dlc = false;
    randomize_identifier = false;
    randomize_data = true;
}


can::Frame::Frame(FrameFlags frameFlags, uint8_t dlc)
{
    frame_flags_ = frameFlags;
    set_dlc(dlc);

    randomize_dlc = false;
    randomize_identifier = true;
    randomize_data = true;
}


can::Frame::Frame(FrameFlags frameFlags)
{
    frame_flags_ = frameFlags;
    dlc_ = 0;
    identifier_ = 0;

    randomize_dlc = true;
    randomize_identifier = true;
    randomize_data = true;
}


can::Frame::Frame(FrameFlags frameFlags, uint8_t dlc, uint8_t *data)
{
    frame_flags_ = frameFlags;
    set_dlc(dlc);
    CopyData(data, data_lenght_);
    identifier_ = 0;

    randomize_dlc = false;
    randomize_identifier = true;
    randomize_data = false;
}

void can::Frame::Randomize()
{
    /* First randomize flags, this gives cosntraints for further randomization */
    frame_flags_.Randomize();

    /*  Due to RTR Flag , Data length might have changed! Update it! */
    set_dlc(dlc_);

    if (randomize_identifier)
    {
        if (frame_flags().is_ide_ == IdentifierType::Extended)
            set_identifer(rand() % (2 ^ 29));
        else
            set_identifer(rand() % (2 ^ 11));
    }

    if (randomize_dlc)
    {
        // Constrain here so that we get reasonable frames for CAN 2.0
        if (frame_flags().is_fdf_ == FrameType::CanFd)
            set_dlc(rand() % 0x9);
        else
            set_dlc(rand() % 0xF);
    }

    if (randomize_data)
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

bool operator==(can::Frame& lhs, can::Frame& rhs)
{
    if (lhs.identifier() != rhs.identifier())
        return false;
    if (lhs.dlc() != rhs.dlc())
        return false;
    if (!(lhs.frame_flags() == rhs.frame_flags()))
        return false;
    for (int i = 0; i < lhs.data_length(); i++)
        if (lhs.data(i) != rhs.data(i))
            return false;
    return true;
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

    assert(!(frame_flags_.is_fdf_ == FrameType::Can2_0 && dataLenght > 8) &&
            "Can't set data length to more than 8 on CAN 2.0 frame");
    
    data_lenght_ = dataLenght;
    dlc_ = ConvertDataLenghtToDlc(dataLenght);
}

void can::Frame::set_identifer(int identifier)
{
    assert((!(frame_flags_.is_ide_ == IdentifierType::Base && identifier >= pow(2.0, 11)),
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
    if (frame_flags_.is_fdf_ == FrameType::Can2_0 && frame_flags_.is_rtr_ == RtrFlag::RtrFrame)
        return 0;

    if (frame_flags_.is_fdf_ == FrameType::Can2_0 && dlc >= 0x8)
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
    std::cout << std::string(80, '*') << std::endl;
    std::cout << "CAN Frame:" << std::endl;
    std::cout << "FDF: " << frame_flags_.is_fdf_ << std::endl;
    std::cout << "IDE: " << frame_flags_.is_ide_ << std::endl;
    if (frame_flags_.is_fdf_ == FrameType::CanFd)
        std::cout << "BRS: " << frame_flags_.is_brs_ << std::endl;
    else
        std::cout << "RTR: " << frame_flags_.is_rtr_ << std::endl;
    std::cout << "DLC: 0x" << std::hex << +dlc_ << std::endl;
    std::cout << "ESI: " << frame_flags_.is_esi_ << std::endl;
    std::cout << "Data field length: " << data_lenght_ << std::endl;
    std::cout << "Identifier: " << std::hex << identifier_ << std::endl;

    std::cout << "Data: ";
    for (int i = 0; i < data_lenght_; i++)
        std::cout << "0x" << std::hex << +data_[i] << " ";
    std::cout << std::endl;

    std::cout << std::string(80, '*') << std::endl;
}