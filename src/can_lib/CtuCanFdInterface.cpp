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

#include <assert.h>

#include "can.h"
#include "Frame.h"
#include "DutInterface.h"
#include "BitTiming.h"

#include "CtuCanFdInterface.h"
#include "../pli_lib/PliComplianceLib.hpp"

/*
 * Directly reference generated C headers in CTU CAN FD main repo!
 */
extern "C" {
    #include "../../../../../driver/ctucanfd_frame.h"
    #include "../../../../../driver/ctucanfd_regs.h"
}


void can::CtuCanFdInterface::Enable()
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.ena = CTU_CAN_ENABLED;

    /*
     * By default forbid that TXT Buffer goes to TX Failed in bus-off. This allows
     * testing reintegration time!
     */
    data.s.tbfbo = 0;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    /** Read number of TXT buffers to do buffer rotation by TX routine correctly! */
    num_txt_buffers_ = (int)MemBusAgentRead16(CTU_CAN_FD_TXTB_INFO);

    /** Set-up TXT Buffer 1 to be used by default */
    cur_txt_buf = 0;
}


void can::CtuCanFdInterface::Disable()
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.ena = CTU_CAN_DISABLED;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
}


void can::CtuCanFdInterface::Reset()
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.rst = 1;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
}


bool can::CtuCanFdInterface::SetFdStandardType(bool isIso)
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    if (isIso)
        data.s.nisofd = ISO_FD;
    else
        data.s.nisofd = NON_ISO_FD;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    return true;
}


bool can::CtuCanFdInterface::SetCanVersion(CanVersion canVersion)
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);

    switch (canVersion)
    {
    case CanVersion::Can_2_0:
        data.s.fde = FDE_DISABLE;
        MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
        return true;
        break;

    case CanVersion::CanFdEnabled:
        data.s.fde = FDE_ENABLE;
        MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
        return true;
        break;

    case CanVersion::CanFdTolerant:
        std::cerr << "CTU CAN FD does not support CAN FD tolerant operation!" << std::endl;
        return false;
        break;
    }
    return false;
}

void can::CtuCanFdInterface::ConfigureBitTiming(can::BitTiming nominal_bit_timing,
                                                can::BitTiming data_bit_timing)
{
    union ctu_can_fd_btr data;
    union ctu_can_fd_btr_fd data_fd;

    data.u32 = 0;
    data.s.brp = nominal_bit_timing.brp_;
    data.s.ph1 = nominal_bit_timing.ph1_;
    data.s.ph2 = nominal_bit_timing.ph2_;
    data.s.sjw = nominal_bit_timing.sjw_;
    data.s.prop = nominal_bit_timing.prop_;
    MemBusAgentWrite32(CTU_CAN_FD_BTR, data.u32);

    data_fd.u32 = 0;
    data_fd.s.brp_fd = data_bit_timing.brp_;
    data_fd.s.ph1_fd = data_bit_timing.ph1_;
    data_fd.s.ph2_fd = data_bit_timing.ph2_;
    data_fd.s.sjw_fd = data_bit_timing.sjw_;
    data_fd.s.prop_fd = data_bit_timing.prop_;
    MemBusAgentWrite32(CTU_CAN_FD_BTR_FD, data_fd.u32);
}


void can::CtuCanFdInterface::ConfigureSsp(SspType ssp_type, int ssp_offset)
{
    union ctu_can_fd_trv_delay_ssp_cfg ssp_cfg;
    ssp_cfg.u32 = 0;

    if (ssp_type == SspType::Disabled)
        ssp_cfg.s.ssp_src = ctu_can_fd_ssp_cfg_ssp_src::SSP_SRC_NO_SSP;
    else if (ssp_type == SspType::MeasuredPlusOffset)
        ssp_cfg.s.ssp_src = ctu_can_fd_ssp_cfg_ssp_src::SSP_SRC_MEAS_N_OFFSET;
    else if (ssp_type == SspType::Offset)
        ssp_cfg.s.ssp_src = ctu_can_fd_ssp_cfg_ssp_src::SSP_SRC_OFFSET;

    ssp_cfg.s.ssp_offset = ssp_offset;
    MemBusAgentWrite32(CTU_CAN_FD_TRV_DELAY, ssp_cfg.u32);
}


void can::CtuCanFdInterface::SendFrame(can::Frame *frame)
{
    union ctu_can_fd_frame_format_w frame_format_word;
    union ctu_can_fd_identifier_w identifier_word;

    /* TXT Buffer address */
    int txt_buffer_address = CTU_CAN_FD_TXTB1_DATA_1;
    switch (cur_txt_buf)
    {
    case 0:
        txt_buffer_address = CTU_CAN_FD_TXTB1_DATA_1;
        break;
    case 1:
        txt_buffer_address = CTU_CAN_FD_TXTB2_DATA_1;
        break;
    case 2:
        txt_buffer_address = CTU_CAN_FD_TXTB3_DATA_1;
        break;
    case 3:
        txt_buffer_address = CTU_CAN_FD_TXTB4_DATA_1;
        break;
    case 4:
        txt_buffer_address = CTU_CAN_FD_TXTB5_DATA_1;
        break;
    case 5:
        txt_buffer_address = CTU_CAN_FD_TXTB6_DATA_1;
        break;
    case 6:
        txt_buffer_address = CTU_CAN_FD_TXTB7_DATA_1;
        break;
    case 7:
        txt_buffer_address = CTU_CAN_FD_TXTB8_DATA_1;
        break;
    default:
        break;
    }

    // Frame format word
    frame_format_word.u32 = 0;
    if (frame->frame_flags().is_fdf() == FrameType::CanFd)
        frame_format_word.s.fdf = ctu_can_fd_frame_format_w_fdf::FD_CAN;
    else
        frame_format_word.s.fdf = ctu_can_fd_frame_format_w_fdf::NORMAL_CAN;

    if (frame->frame_flags().is_ide() == IdentifierType::Extended)
        frame_format_word.s.ide = ctu_can_fd_frame_format_w_ide::EXTENDED;
    else
        frame_format_word.s.ide = ctu_can_fd_frame_format_w_ide::BASE;

    if (frame->frame_flags().is_rtr() == RtrFlag::RtrFrame)
        frame_format_word.s.rtr = ctu_can_fd_frame_format_w_rtr::RTR_FRAME;
    else
        frame_format_word.s.rtr = ctu_can_fd_frame_format_w_rtr::NO_RTR_FRAME;

    if (frame->frame_flags().is_brs() == BrsFlag::Shift)
        frame_format_word.s.brs = ctu_can_fd_frame_format_w_brs::BR_SHIFT;
    else
        frame_format_word.s.brs = ctu_can_fd_frame_format_w_brs::BR_NO_SHIFT;

    if (frame->frame_flags().is_esi() == EsiFlag::ErrorActive)
        frame_format_word.s.esi_rsv = ctu_can_fd_frame_format_w_esi_rsv::ESI_ERR_ACTIVE;
    else
        frame_format_word.s.esi_rsv = ctu_can_fd_frame_format_w_esi_rsv::ESI_ERR_PASIVE;

    frame_format_word.s.dlc = frame->dlc();

    // Identifier word
    identifier_word.u32 = 0;

    if (frame->frame_flags().is_ide() == IdentifierType::Extended)
    {
        identifier_word.s.identifier_base =
            (((uint32_t)frame->identifier()) >> 18) & 0x7FF;
        identifier_word.s.identifier_ext =
            ((uint32_t)frame->identifier()) & 0x3FFFF;
    } else {
        identifier_word.s.identifier_base =
            ((uint32_t)frame->identifier() & 0x7FF);
        identifier_word.s.identifier_ext = 0;
    }

    // Write first 4 words to TXT Buffer. Timestamp 0 -> send immediately!
    MemBusAgentWrite32(txt_buffer_address, frame_format_word.u32);
    MemBusAgentWrite32(txt_buffer_address + 0x4, identifier_word.u32);
    MemBusAgentWrite32(txt_buffer_address + 0x8, 0);
    MemBusAgentWrite32(txt_buffer_address + 0xC, 0);
    txt_buffer_address += 0x10;

    int num_data_words = (frame->data_length() - 1) / 4 + 1;
    std::cout << "FRAME DATA LENGTH: " << frame->data_length();

    for (int i = 0; i < num_data_words; i++)
    {
        uint32_t data_wrd = 0;
        for (int j = 0; j < 4; j++)
            data_wrd |= (frame->data(i * 4 + j)) << (j * 8);
        MemBusAgentWrite32(txt_buffer_address, data_wrd);
        txt_buffer_address += 0x4;
    }

    // Give command to chosen buffer
    MemBusAgentWrite32(CTU_CAN_FD_TX_COMMAND, 0x2 | (1 << (cur_txt_buf + 8)));

    // Iterate TXT Buffers - Indexed from 0 - Move to next buffer
    // txt_buf_nr is set by Enable function to 0 (TXT Buffer 0), so that first call to SendFrame
    // will send frame via TXT Buffer 0, which is by default highest priority. Therefore even if
    // two frames are sent at once, they are always transmitted by DUT in order in which SendFrame
    // has been called!
    cur_txt_buf += 1;
    cur_txt_buf %= num_txt_buffers_;

    assert(cur_txt_buf >= 0 && cur_txt_buf < num_txt_buffers_);
}


can::Frame can::CtuCanFdInterface::ReadFrame()
{
    union ctu_can_fd_frame_format_w frame_format_word;
    union ctu_can_fd_identifier_w identifier_word;
    uint32_t data_word;
    int rwcnt;
    int identifier;
    uint8_t data[64];
    memset(data, 0, sizeof(data));

    // Flags
    FrameType is_fdf;
    IdentifierType is_ide;
    RtrFlag is_rtr;
    BrsFlag is_brs;
    EsiFlag is_esi;

    frame_format_word.u32 = MemBusAgentRead32(CTU_CAN_FD_RX_DATA);
    identifier_word.u32 = MemBusAgentRead32(CTU_CAN_FD_RX_DATA);

    // Skip Timestamp words
    data_word = MemBusAgentRead32(CTU_CAN_FD_RX_DATA);
    data_word = MemBusAgentRead32(CTU_CAN_FD_RX_DATA);
    rwcnt = frame_format_word.s.rwcnt;

    // Set flags
    if (frame_format_word.s.fdf == ctu_can_fd_frame_format_w_fdf::FD_CAN)
        is_fdf = FrameType::CanFd;
    else
        is_fdf = FrameType::Can2_0;

    if (frame_format_word.s.ide == ctu_can_fd_frame_format_w_ide::EXTENDED)
        is_ide = IdentifierType::Extended;
    else
        is_ide = IdentifierType::Base;

    if (frame_format_word.s.rtr == ctu_can_fd_frame_format_w_rtr::RTR_FRAME)
        is_rtr = RtrFlag::RtrFrame;
    else
        is_rtr = RtrFlag::DataFrame;

    if (frame_format_word.s.brs == ctu_can_fd_frame_format_w_brs::BR_SHIFT)
        is_brs = BrsFlag::Shift;
    else
        is_brs = BrsFlag::DontShift;

    if (frame_format_word.s.esi_rsv == ctu_can_fd_frame_format_w_esi_rsv::ESI_ERR_ACTIVE)
        is_esi = EsiFlag::ErrorActive;
    else
        is_esi = EsiFlag::ErrorPassive;

    FrameFlags frameFlags = FrameFlags(is_fdf, is_ide, is_rtr, is_brs, is_esi);

    // Read identifier
    if (is_ide == IdentifierType::Extended)
        identifier = (identifier_word.s.identifier_base << 18) |
                     identifier_word.s.identifier_ext;
    else
        identifier = identifier_word.s.identifier_base;

    // Read data
    for (int i = 0; i < rwcnt - 3; i++)
    {
        data_word = MemBusAgentRead32(CTU_CAN_FD_RX_DATA);

        for (int j = 0; j < 4; j++)
        {
            data[(i * 4) + j] = (uint8_t)(data_word & 0xFF);
            data_word >>= 8;
        }
    }

    return Frame(frameFlags, (uint8_t)frame_format_word.s.dlc, identifier, data);
}


bool can::CtuCanFdInterface::HasRxFrame()
{
    union ctu_can_fd_rx_status_rx_settings rx_status;
    rx_status.u32 = MemBusAgentRead32(CTU_CAN_FD_RX_STATUS);

    assert(!(rx_status.s.rxe == 1 && rx_status.s.rxfrc > 0));

    if (rx_status.s.rxe == 1)
        return false;
    return true;
}

int can::CtuCanFdInterface::GetRec()
{
    union ctu_can_fd_rec_tec data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_REC);
    return data.s.rec_val;
}


int can::CtuCanFdInterface::GetTec()
{
    union ctu_can_fd_rec_tec data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_REC);
    return data.s.tec_val;
}


void can::CtuCanFdInterface::SetRec(int rec)
{
    // Enable test-mode otherwise we will not be able to change REC or TEC!
    union ctu_can_fd_ctr_pres ctr_pres;
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.tstm = 1;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    ctr_pres.u32 = 0;
    ctr_pres.s.prx = 1;
    ctr_pres.s.ctpv = rec;

    MemBusAgentWrite32(CTU_CAN_FD_CTR_PRES, ctr_pres.u32);
}


void can::CtuCanFdInterface::SetTec(int tec)
{
    // Enable test-mode otherwise we will not be able to change REC or TEC!
    union ctu_can_fd_ctr_pres ctr_pres;
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.tstm = 1;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    ctr_pres.u32 = 0;
    ctr_pres.s.ptx = 1;
    ctr_pres.s.ctpv = tec;

    MemBusAgentWrite32(CTU_CAN_FD_CTR_PRES, ctr_pres.u32);
}


void can::CtuCanFdInterface::SetErrorState(FaultConfinementState error_state)
{
    // Enable test-mode otherwise we will not be able to change REC or TEC!
    union ctu_can_fd_ctr_pres ctr_pres;
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.tstm = 1;
    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);

    ctr_pres.u32 = 0;
    ctr_pres.s.ptx = 1;
    ctr_pres.s.prx = 1;

    switch (error_state)
    {
    case FaultConfinementState::ErrorActive:
        ctr_pres.s.ctpv = 0;
        break;

    case FaultConfinementState::ErrorPassive:
        ctr_pres.s.ctpv = 150;
        break;

    case FaultConfinementState::BusOff:
        ctr_pres.s.ctpv = 260;
        break;

    default:
        break;
    }

    MemBusAgentWrite32(CTU_CAN_FD_CTR_PRES, ctr_pres.u32);
}


can::FaultConfinementState can::CtuCanFdInterface::GetErrorState()
{
    union ctu_can_fd_ewl_erp_fault_state data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_EWL);

    printf("READ FAULT VALUE: 0x%x\n", data.u32);
    // HW should signal always only one state!

    if (data.s.bof == 1)
        return FaultConfinementState::BusOff;
    if (data.s.era == 1)
        return FaultConfinementState::ErrorActive;
    if (data.s.erp == 1)
        return FaultConfinementState::ErrorPassive;

    // If we get here, something is wrong with HW!
    return FaultConfinementState::Invalid;
}


bool can::CtuCanFdInterface::ConfigureProtocolException(bool enable)
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.pex = (enable) ? 1 : 0;

    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
    return true;
}


bool can::CtuCanFdInterface::ConfigureOneShot(bool enable)
{
    union ctu_can_fd_mode_settings data;
    data.u32 = MemBusAgentRead32(CTU_CAN_FD_MODE);
    data.s.rtrle = enable;
    data.s.rtrth = 0;

    MemBusAgentWrite32(CTU_CAN_FD_MODE, data.u32);
    return true;
}


void can::CtuCanFdInterface::SendReintegrationRequest()
{
    union ctu_can_fd_command data;
    data.u32 = 0;
    data.s.ercrst = 1;

    MemBusAgentWrite32(CTU_CAN_FD_COMMAND, data.u32);
}
