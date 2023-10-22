#ifndef CTU_CAN_FD_INTERFACE_H
#define CTU_CAN_FD_INTERFACE_H
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

#include "can.h"
#include "Frame.h"
#include "DutInterface.h"
#include "BitTiming.h"

extern "C" {

    /**
     * @class CtuCanFdInterface
     * @namespace can
     *
     * Implementation of DutInterface for CTU CAN FD IP Core.
     *
     */
    class can::CtuCanFdInterface : public can::DutInterface
    {
        /**
         * All functions implement virtual interface "DutInterface" which allows
         * writing tests in DUT independent manner! All configuration of DUT shall
         * be executed via functions of this interface, not by manuall access to
         * DUT!
         */
        public:
            void Enable();
            void Disable();
            void Reset();
            bool SetFdStandardType(bool isIso);
            bool SetCanVersion(CanVersion canVersion);
            void ConfigureBitTiming(can::BitTiming nominal_bit_timing,
                                    can::BitTiming data_bit_timing);
            void ConfigureSsp(SspType sspType, int sspOffset);
            void SendFrame(can::Frame *frame);
            can::Frame ReadFrame();
            bool HasRxFrame();
            int GetRec();
            int GetTec();
            void SetRec(int rec);
            void SetTec(int tec);
            void SetErrorState(can::FaultConfinementState errorState);
            can::FaultConfinementState GetErrorState();
            bool ConfigureProtocolException(bool enable);
            bool ConfigureOneShot(bool enable);
            void SendReintegrationRequest();

            /* Number of TXT buffers. Read by "Enable" */
            int num_txt_buffers_;

            /* Currently used TXT buffer */
            unsigned int cur_txt_buf;
    };
}

#endif