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

#include <assert.h>

#include "can.h"
#include "Frame.h"
#include "DutInterface.h"
#include "BitTiming.h"

#include "CtuCanFdInterface.h"
#include "../vpi_lib/vpiComplianceLib.hpp"

/*
 * Directly reference generated C headers in CTU CAN FD main repo!
 */
extern "C" {
    #include "../../../../driver/ctu_can_fd_frame.h"
    #include "../../../../driver/ctu_can_fd_regs.h"
}


void can::CtuCanFdInterface::enable()
{
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.ena = CTU_CAN_ENABLED;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
}


void can::CtuCanFdInterface::disable()
{
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.ena = CTU_CAN_DISABLED;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
}


void can::CtuCanFdInterface::reset()
{
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.rst = 1;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
}


bool can::CtuCanFdInterface::setFdStandardType(bool isIso)
{
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    if (isIso)
        data.s.nisofd = ISO_FD;
    else
        data.s.nisofd = NON_ISO_FD;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    return true;
}


bool can::CtuCanFdInterface::setCanVersion(CanVersion canVersion)
{
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);

    switch (canVersion)
    {
    case CAN_2_0_VERSION:
        data.s.fde = FDE_DISABLE;
        memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
        return true;
        break;

    case CAN_FD_ENABLED_VERSION:
        data.s.fde = FDE_ENABLE;
        memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
        return true;
        break;

    case CAN_FD_TOLERANT_VERSION:
        std::cerr << "CTU CAN FD does not support CAN FD tolerant operation!" <<
        std::endl;
        return false;
        break;
    }
}



void can::CtuCanFdInterface::configureBitTiming(can::BitTiming nominalBitTiming,
                                                can::BitTiming dataBitTiming)
{
    union ctu_can_fd_btr data;
    union ctu_can_fd_btr_fd dataFd;

    data.u32 = 0;
    data.s.brp = nominalBitTiming.brp;
    data.s.ph1 = nominalBitTiming.ph1;
    data.s.ph2 = nominalBitTiming.ph2;
    data.s.sjw = nominalBitTiming.sjw;
    data.s.prop = nominalBitTiming.prop;
    memBusAgentWrite32(CTU_CAN_FD_BTR, data.u32);

    dataFd.u32 = 0;
    dataFd.s.brp_fd = dataBitTiming.brp;
    dataFd.s.ph1_fd = dataBitTiming.ph1;
    dataFd.s.ph2_fd = dataBitTiming.ph2;
    dataFd.s.sjw_fd = dataBitTiming.sjw;
    dataFd.s.prop_fd = dataBitTiming.prop;
    memBusAgentWrite32(CTU_CAN_FD_BTR_FD, dataFd.u32);
}


void can::CtuCanFdInterface::sendFrame(can::Frame frame)
{
    union ctu_can_fd_frame_form_w frameFormatWord;
    union ctu_can_fd_identifier_w identifierWord;

    // Do TXT Buffer rotation! Each time frame is sent, TXT Buffer is
    // incremented by 1!
    // We don't intend this to be thread safe, so we have no trouble in
    // having statis variable here!
    static int txt_buf_nr;
    if (txt_buf_nr > 4 || txt_buf_nr < 1)
        txt_buf_nr = 1;
    else
        txt_buf_nr++;

    // TXT Buffer address
    int txtBufferAddress;
    switch (txt_buf_nr)
    {
    case 1:
        txtBufferAddress = CTU_CAN_FD_TXTB1_DATA_1;
        break;
    case 2:
        txtBufferAddress = CTU_CAN_FD_TXTB2_DATA_1;
        break;
    case 3:
        txtBufferAddress = CTU_CAN_FD_TXTB3_DATA_1;
        break;
    case 4:
        txtBufferAddress = CTU_CAN_FD_TXTB4_DATA_1;
        break;
    default:
        break;
    }

    // Frame format word
    frameFormatWord.u32 = 0;    
    if (frame.getFrameFlags().isFdf_ == can::CAN_FD)
        frameFormatWord.s.fdf = ctu_can_fd_frame_form_w_fdf::FD_CAN;
    else
        frameFormatWord.s.fdf = ctu_can_fd_frame_form_w_fdf::NORMAL_CAN;

    if (frame.getFrameFlags().isIde_ == can::EXTENDED_IDENTIFIER)
        frameFormatWord.s.ide = ctu_can_fd_frame_form_w_ide::EXTENDED;
    else
        frameFormatWord.s.ide = ctu_can_fd_frame_form_w_ide::BASE;

    if (frame.getFrameFlags().isRtr_ == can::RTR_FRAME)
        frameFormatWord.s.rtr = ctu_can_fd_frame_form_w_rtr::RTR_FRAME;
    else
        frameFormatWord.s.rtr = ctu_can_fd_frame_form_w_rtr::NO_RTR_FRAME;

    if (frame.getFrameFlags().isBrs_ == can::BIT_RATE_SHIFT)
        frameFormatWord.s.brs = ctu_can_fd_frame_form_w_brs::BR_SHIFT;
    else
        frameFormatWord.s.brs = ctu_can_fd_frame_form_w_brs::BR_NO_SHIFT;

    if (frame.getFrameFlags().isEsi_ == can::ESI_ERROR_ACTIVE)
        frameFormatWord.s.esi_rsv = ctu_can_fd_frame_form_w_esi_rsv::ESI_ERR_ACTIVE;
    else
        frameFormatWord.s.esi_rsv = ctu_can_fd_frame_form_w_esi_rsv::ESI_ERR_PASIVE;

    frameFormatWord.s.dlc = frame.getDlc();

    // Identifier word
    identifierWord.u32 = 0;

    if (frame.getFrameFlags().isIde_ == can::EXTENDED_IDENTIFIER)
    {
        identifierWord.s.identifier_base =
            (((uint32_t)frame.getIdentifier()) >> 18) & 0x7FF;
        identifierWord.s.identifier_ext =
            ((uint32_t)frame.getIdentifier()) & 0x3FFFF;
    } else {
        identifierWord.s.identifier_base =
            ((uint32_t)frame.getIdentifier() & 0x7FF);
        identifierWord.s.identifier_ext = 0;
    }

    // Write first 4 words to TXT Buffer. Timestamp 0 -> send immediately!
    memBusAgentWrite32(txtBufferAddress++, frameFormatWord.u32);
    memBusAgentWrite32(txtBufferAddress++, identifierWord.u32);
    memBusAgentWrite32(txtBufferAddress++, 0);
    memBusAgentWrite32(txtBufferAddress++, 0);

    for (int i = 0; i < frame.getDataLenght() / 4; i++)
    {
        uint32_t dataWrd = 0;
        for (int j = 0; j < 4; j++)
            dataWrd |= (frame.getData(i * 4 + j)) << (j * 8);
        memBusAgentWrite32(txtBufferAddress++, dataWrd);
    }
}


can::Frame can::CtuCanFdInterface::readFrame()
{
    union ctu_can_fd_frame_form_w frameFormatWord;
    union ctu_can_fd_identifier_w identifierWord;
    uint32_t dataWord;
    int rwcnt;
    int identifier;
    uint8_t data[64];
    memset(data, 0, sizeof(data));

    // Flags
    FlexibleDataRate isFdf;
    ExtendedIdentifier isIde;
    RemoteTransmissionRequest isRtr;
    BitRateShift isBrs;
    ErrorStateIndicator isEsi;
    
    frameFormatWord.u32 = memBusAgentRead32(CTU_CAN_FD_RX_DATA);
    identifierWord.u32 = memBusAgentRead32(CTU_CAN_FD_RX_DATA);

    // Skip Timestamp words
    dataWord = memBusAgentRead32(CTU_CAN_FD_RX_DATA);
    dataWord = memBusAgentRead32(CTU_CAN_FD_RX_DATA);
    rwcnt = frameFormatWord.s.rwcnt;

    // Set flags
    if (frameFormatWord.s.fdf == ctu_can_fd_frame_form_w_fdf::FD_CAN)
        isFdf = FlexibleDataRate::CAN_FD;
    else
        isFdf = FlexibleDataRate::CAN_2_0;
    
    if (frameFormatWord.s.ide == ctu_can_fd_frame_form_w_ide::EXTENDED)
        isIde = ExtendedIdentifier::EXTENDED_IDENTIFIER;
    else
        isIde = ExtendedIdentifier::BASE_IDENTIFIER;
    
    if (frameFormatWord.s.rtr == ctu_can_fd_frame_form_w_rtr::RTR_FRAME)
        isRtr = RemoteTransmissionRequest::RTR_FRAME;
    else
        isRtr = RemoteTransmissionRequest::DATA_FRAME;

    if (frameFormatWord.s.brs == ctu_can_fd_frame_form_w_brs::BR_SHIFT)
        isBrs = BitRateShift::BIT_RATE_SHIFT;
    else
        isBrs = BitRateShift::BIT_RATE_DONT_SHIFT;

    if (frameFormatWord.s.esi_rsv == ctu_can_fd_frame_form_w_esi_rsv::ESI_ERR_ACTIVE)
        isEsi = ErrorStateIndicator::ESI_ERROR_ACTIVE;
    else
        isEsi = ErrorStateIndicator::ESI_ERROR_PASSIVE;

    FrameFlags frameFlags = FrameFlags(isFdf, isIde, isRtr, isBrs, isEsi);

    // Read identifier
    if (isIde == ExtendedIdentifier::EXTENDED_IDENTIFIER)
        identifier = (identifierWord.s.identifier_base << 18) |
                     identifierWord.s.identifier_ext;
    else
        identifier = identifierWord.s.identifier_base;

    // Read data
    for (int i = 0; i < rwcnt - 3; i++)
    {
        dataWord = memBusAgentRead32(CTU_CAN_FD_RX_DATA);

        for (int j = 0; j < 4; j++)
        {
            data[(i * 4) + j] = (uint8_t)(dataWord & 0xFF);
            dataWord >>= 8;
        }
    }

    return Frame(frameFlags, (uint8_t)frameFormatWord.s.dlc, identifier, data);
}


bool can::CtuCanFdInterface::hasRxFrame()
{
    union ctu_can_fd_rx_status_rx_settings rxStatus;
    rxStatus.u32 = memBusAgentRead32(CTU_CAN_FD_RX_STATUS);

    assert(!(rxStatus.s.rxe == 1 && rxStatus.s.rxfrc > 0));

    if (rxStatus.s.rxe == 1)
        return false;
    return true;
}

int can::CtuCanFdInterface::getRec()
{
    union ctu_can_fd_rec_tec data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_REC);
    return data.s.rec_val;
}


int can::CtuCanFdInterface::getTec()
{
    union ctu_can_fd_rec_tec data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_REC);
    return data.s.tec_val;
}

        
void can::CtuCanFdInterface::setRec(int rec)
{
    // Enable test-mode otherwise we will not be able to change REC or TEC!
    union ctu_can_fd_ctr_pres ctrPres;
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.tstm = 1;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    ctrPres.u32 = 0;
    ctrPres.s.prx = 1;
    ctrPres.s.ctpv = rec;

    memBusAgentWrite32(CTU_CAN_FD_CTR_PRES, ctrPres.u32);
}
        
    
void can::CtuCanFdInterface::setTec(int tec)
{
    // Enable test-mode otherwise we will not be able to change REC or TEC!
    union ctu_can_fd_ctr_pres ctrPres;
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.tstm = 1;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    ctrPres.u32 = 0;
    ctrPres.s.ptx = 1;
    ctrPres.s.ctpv = tec;

    memBusAgentWrite32(CTU_CAN_FD_CTR_PRES, ctrPres.u32);
}


void can::CtuCanFdInterface::setErrorState(ErrorState errorState)
{
    // Enable test-mode otherwise we will not be able to change REC or TEC!
    union ctu_can_fd_ctr_pres ctrPres;
    union ctu_can_fd_mode_settings data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.tstm = 1;
    memBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    ctrPres.u32 = 0;
    ctrPres.s.ptx = 1;
    ctrPres.s.prx = 1;

    switch (errorState)
    {
    case ERROR_ACTIVE:
        ctrPres.s.ctpv = 0;
        break;

    case ERROR_PASSIVE:
        ctrPres.s.ctpv = 130;
        break;

    case BUS_OFF:
        ctrPres.s.ctpv = 260;
        break;
    }

    memBusAgentWrite32(CTU_CAN_FD_CTR_PRES, ctrPres.u32);
}


can::ErrorState can::CtuCanFdInterface::getErrorState()
{
    union ctu_can_fd_ewl_erp_fault_state data;
    data.u32 = memBusAgentRead32(CTU_CAN_FD_EWL);

    printf("READ FAULT VALUE: 0x%x\n", data.u32);
    // HW should signal always only one state!
    uint8_t stateBits = data.u32 & 0x7;
    //assert(stateBits == 1 || stateBits == 2 || stateBits == 4);

    if (data.s.bof == 1)
        return can::BUS_OFF;
    if (data.s.era == 1)
        return can::ERROR_ACTIVE;
    if (data.s.erp == 1)
        return can::BUS_OFF;

    // We should never get here!
    assert(false);
}