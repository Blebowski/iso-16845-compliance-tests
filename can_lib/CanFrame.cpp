/*
 * TODO: License
 */

#include <iostream>
#include <cmath>

#include "CanFrame.h"


///////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////

CanFrame::CanFrame()
{
    isFdf_ = CAN_2_0;
    isIde_ = BASE_IDENTIFIER;
    isRtr_ = DATA_FRAME;
    isBrs_ = BIT_RATE_DONT_SHIFT;
    isEsi_ = ESI_ERROR_ACTIVE;
    dlc_ = 0;
    identifier_ = 0;
    dataLenght_ = 0;

    for (int i = 0; i < 64; i++)
        data_[i] = 0x0;
}


CanFrame::CanFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                   RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                   ErrorStateIndicator isEsi, uint8_t dlc, int identifier,
                   uint8_t *data)
{
    setIde(isIde);
    setFdf(isFdf);
    setBrs(isBrs);
    setRtr(isRtr);
    setEsi(isEsi);

    setDlc(dlc);
    setIdentifer(identifier);
    copyData(data, dataLenght_);
}

CanFrame::CanFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                   RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                   ErrorStateIndicator isEsi, int dataLength, int identifier,
                   uint8_t *data)
{
    setIde(isIde);
    setFdf(isFdf);
    setBrs(isBrs);
    setRtr(isRtr);
    setEsi(isEsi);

    setDataLenght(dataLength);
    setIdentifer(identifier);
    copyData(data, dataLenght_);
}

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

FlexibleDataRate CanFrame::getFdf()
{
    return isFdf_;
}

ExtendedIdentifier CanFrame::getIde()
{
    return isIde_;
}

RemoteTransmissionRequest CanFrame::getRtr()
{
    return isRtr_;
}

BitRateShift CanFrame::getBrs()
{
    return isBrs_;
}

ErrorStateIndicator CanFrame::getEsi()
{
    return isEsi_;
}

uint8_t CanFrame::getDlc()
{
    return dlc_;
}

int CanFrame::getDataLenght()
{
    return dataLenght_;
}

int CanFrame::getIdentifier()
{
    return identifier_;
}

uint8_t* CanFrame::getData()
{
    return data_;
}

uint8_t CanFrame::getData(int index)
{
    return data_[index];
}


///////////////////////////////////////////////////////////////////////////////
// Setters
///////////////////////////////////////////////////////////////////////////////

void CanFrame::setFdf(FlexibleDataRate isFdf)
{
    if (isRtr_ == RTR_FRAME){
        std::cerr << "Can't set FDF flag on RTR frame" << std::endl;
        return;
    }
    isFdf_ = isFdf;
}

void CanFrame::setIde(ExtendedIdentifier isIde)
{
    isIde_ = isIde;
}

void CanFrame::setRtr(RemoteTransmissionRequest isRtr)
{
    if (isFdf_ == CAN_FD && isRtr == RTR_FRAME){
        std::cerr << "Can't set RTR flag on CAN FD frame" << std::endl;
        return;
    }
    isRtr_ = isRtr;
}

void CanFrame::setBrs(BitRateShift isBrs)
{
    if (isFdf_ == CAN_2_0){
        std::cerr << "Can't set BRS flag on CAN 2.0 frame" << std::endl;
        return;
    }
    isBrs_ = isBrs;
}

void CanFrame::setEsi(ErrorStateIndicator isEsi)
{
    if (isFdf_ == CAN_2_0){
        std::cerr << "Can't set ESI flag on CAN 2.0 frame" << std::endl;
        return;
    }
    isEsi_ = isEsi;
}

void CanFrame::setDlc(uint8_t dlc)
{
    if (dlc > 16){
        std::cerr << "Can't set DLC higher than 16" << std::endl;
        return;
    }
    dlc_ = dlc;
    dataLenght_ = convertDlcToDataLenght(dlc);
}

bool CanFrame::setDataLenght(int dataLenght)
{
    if (!isValidDataLength(dataLenght)){
        std::cerr << "Can't set data length: " << dataLenght << std::endl;
        return false;
    }

    if (isFdf_ == CAN_2_0 && dataLenght > 8){
        std::cerr << "Can't set data length: " << dataLenght <<
                     " to more than 8 on CAN 2.0 frame";
        return false;
    }

    dataLenght_ = dataLenght;
    dlc_ = convertDataLenghtToDlc(dataLenght);

    return true;
}

void CanFrame::setIdentifer(int identifier)
{
    if (isIde_ == BASE_IDENTIFIER && identifier >= pow(2.0, 11)){
        std::cerr << "Can't set Base identifier larger than 2^11" << std::endl;
        return;
    } else if (identifier >= pow(2.0, 29)){
        std::cerr << "Can't set Extended identifier larger than 2^29" << std::endl;
        return;
    }
    identifier_ = identifier;
}

void CanFrame::copyData(uint8_t *data, int dataLen)
{
    if (data == 0){
        std::cerr << "Null pointer: source data" << std::endl;
        return;
    }

    for (int i = 0; i < dataLen; i++)
        data_[i] = data[i];
}

int CanFrame::convertDlcToDataLenght(uint8_t dlc)
{
    if (isFdf_ == CAN_2_0 && dlc >= 0x8)
        return 0x8;

    for (int i = 0; i < 16; i++){
        if (uint8_t(dlcToDataLenghtTable_[i][0]) == dlc)
            return dlcToDataLenghtTable_[i][0];
    }
    return -1;
}

uint8_t CanFrame::convertDataLenghtToDlc(int dataLenght)
{
    for (int i = 0; i < 16; i++){
        if (uint8_t(dlcToDataLenghtTable_[i][1]) == dataLenght)
            return dlcToDataLenghtTable_[i][0];
    }
    return -1;
}

bool CanFrame::isValidDataLength(int dataLenght)
{
    for (int i = 0; i < 16; i++)
        if (dlcToDataLenghtTable_[i][1] == dataLenght)
            return true;
    return false;
}

void CanFrame::print()
{
    std::cout << std::string(80, '*') << std::endl;
    std::cout << "CAN Frame:" << std::endl;
    std::cout << "FDF: " << isFdf_ << std::endl;
    std::cout << "IDE: " << isIde_ << std::endl;
    if (isFdf_ == CAN_FD)
        std::cout << "BRS: " << isBrs_ << std::endl;
    else
        std::cout << "RTR: " << isRtr_ << std::endl;
    std::cout << "DLC: " << (int)dlc_ << std::endl;
    std::cout << "Data field length: " << dataLenght_ << std::endl;
    std::cout << "Identifier: " << identifier_ << std::endl;
    std::cout << "Data: ";
    for (int i = 0; i < dataLenght_; i++)
        printf("0x%u", data_[i]);
    std::cout << std::endl;
    std::cout << std::string(80, '*') << std::endl;
}