/*
 * TODO: License
 */

#include <iostream>
#include <cmath>

#include "Frame.h"
#include "FrameFlags.h"


///////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////

can::Frame::Frame()
{
    frameFlags_ = FrameFlags();
    dlc_ = 0;
    identifier_ = 0;
    dataLenght_ = 0;

    for (int i = 0; i < 64; i++)
        data_[i] = 0x0;
}


can::Frame::Frame(FrameFlags frameFlags, uint8_t dlc, int identifier,
                  uint8_t *data)
{
    frameFlags_ = frameFlags;    
    setIdentifer(identifier);
    setDlc(dlc);
    copyData(data, dataLenght_);
}

can::Frame::Frame(FrameFlags frameFlags, int dataLength, int identifier,
                  uint8_t *data)
{
    frameFlags_ = frameFlags;
    setDataLenght(dataLength);    
    setIdentifer(identifier);
    copyData(data, dataLenght_);
}

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

can::FrameFlags can::Frame::getFrameFlags()
{
    return frameFlags_;
}

uint8_t can::Frame::getDlc()
{
    return dlc_;
}

int can::Frame::getDataLenght()
{
    return dataLenght_;
}

int can::Frame::getIdentifier()
{
    return identifier_;
}

uint8_t* can::Frame::getData()
{
    return data_;
}

uint8_t can::Frame::getData(int index)
{
    return data_[index];
}


///////////////////////////////////////////////////////////////////////////////
// Setters
///////////////////////////////////////////////////////////////////////////////

void can::Frame::setFrameFlags(FrameFlags frameFlags)
{
    frameFlags_ = frameFlags;
}

void can::Frame::setDlc(uint8_t dlc)
{
    if (dlc > 16){
        std::cerr << "Can't set DLC higher than 16" << std::endl;
        return;
    }
    dlc_ = dlc;
    dataLenght_ = convertDlcToDataLenght(dlc);
}

bool can::Frame::setDataLenght(int dataLenght)
{
    if (!isValidDataLength(dataLenght)){
        std::cerr << "Can't set data length: " << dataLenght << std::endl;
        return false;
    }

    if (frameFlags_.isFdf_ == CAN_2_0 && dataLenght > 8){
        std::cerr << "Can't set data length: " << dataLenght <<
                     " to more than 8 on CAN 2.0 frame";
        return false;
    }

    dataLenght_ = dataLenght;
    dlc_ = convertDataLenghtToDlc(dataLenght);

    return true;
}

void can::Frame::setIdentifer(int identifier)
{
    if (frameFlags_.isIde_ == BASE_IDENTIFIER && identifier >= pow(2.0, 11)){
        std::cerr << "Can't set Base identifier larger than 2^11" << std::endl;
        return;
    } else if (identifier >= pow(2.0, 29)){
        std::cerr << "Can't set Extended identifier larger than 2^29" << std::endl;
        return;
    }
    identifier_ = identifier;
}

void can::Frame::copyData(uint8_t *data, int dataLen)
{
    if (data == 0){
        std::cerr << "Null pointer: source data" << std::endl;
        return;
    }

    for (int i = 0; i < dataLen; i++)
        data_[i] = data[i];
}

int can::Frame::convertDlcToDataLenght(uint8_t dlc)
{
    if (frameFlags_.isFdf_ == CAN_2_0 && dlc >= 0x8)
        return 0x8;

    for (int i = 0; i < 16; i++){
        if (uint8_t(dlcToDataLenghtTable_[i][0]) == dlc)
            return dlcToDataLenghtTable_[i][0];
    }
    return -1;
}

uint8_t can::Frame::convertDataLenghtToDlc(int dataLenght)
{
    for (int i = 0; i < 16; i++){
        if (uint8_t(dlcToDataLenghtTable_[i][1]) == dataLenght)
            return dlcToDataLenghtTable_[i][0];
    }
    return -1;
}

bool can::Frame::isValidDataLength(int dataLenght)
{
    for (int i = 0; i < 16; i++)
        if (dlcToDataLenghtTable_[i][1] == dataLenght)
            return true;
    return false;
}

void can::Frame::print()
{
    std::cout << std::string(80, '*') << std::endl;
    std::cout << "CAN Frame:" << std::endl;
    std::cout << "FDF: " << frameFlags_.isFdf_ << std::endl;
    std::cout << "IDE: " << frameFlags_.isIde_ << std::endl;
    if (frameFlags_.isFdf_ == CAN_FD)
        std::cout << "BRS: " << frameFlags_.isBrs_ << std::endl;
    else
        std::cout << "RTR: " << frameFlags_.isRtr_ << std::endl;
    std::cout << "DLC: " << (int)dlc_ << std::endl;
    std::cout << "Data field length: " << dataLenght_ << std::endl;
    std::cout << "Identifier: " << identifier_ << std::endl;
    std::cout << "Data: ";
    for (int i = 0; i < dataLenght_; i++)
        printf("0x%u", data_[i]);
    std::cout << std::endl;
    std::cout << std::string(80, '*') << std::endl;
}