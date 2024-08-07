#ifndef DUT_INTERFACE_H
#define DUT_INTERFACE_H
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

/**
 * @class DutInterface
 * @namespace can
 *
 * Generic DUT interface. Tests are using implementations of this interface to
 * operate on DUT.
 *
 */
class can::DutInterface
{
    public:
        virtual ~DutInterface(){};

        /**
         * Enables DUT to operate on CAN bus. After enabling, DUT starts integrating.
         */
        virtual void Enable() = 0;

        /**
         * Disables DUT.
         */
        virtual void Disable() = 0;

        /**
         * Resets DUT.
         */
        virtual void Reset() = 0;

        /**
         * Configures CAN FD standard type to be used by DUT.
         * @param is_iso true if ISO CAN FD shall be used, false otherwise
         * @returns true if succesfull, false otherwise (e.g. non-iso is not supported by DUT)
         */
        virtual bool SetFdStandardType(bool is_iso) = 0;

        /**
         * Configures CAN version supported by DUT.
         * @param can_version version to be configured.
         * @returns true if succesfull, false otherwise (e.g. version is not supported)
         */
        virtual bool SetCanVersion(CanVersion can_version) = 0;

        /**
         * Configures Bit timing on CAN bus.
         * @param nominal_bit_timing Bit timing parameters for nominal bit rate
         * @param data_bit_timing Bit timing parameters for data bit rate
         */
        virtual void ConfigureBitTiming(BitTiming nbt, BitTiming dbt) = 0;

        /**
         * Configures secondary sampling point.
         * @param ssp_type Type of secondary sampling to be used.
         * @param ssp_offset Offset to be used if ssp_type contains offset.
         */
        virtual void ConfigureSsp(SspType ssp_type, int ssp_offset) = 0;

        /**
         * Send frame by a DUT. Returns when frame is issued for sending, does not
         * wait till frame was sent.
         * @param frame Pointer to frame to be sent.
         */
        virtual void SendFrame(Frame *frame) = 0;

        /**
         * Reads received frame from DUT.
         * @returns Received frame.
         */
        virtual Frame ReadFrame() = 0;

        /**
         * Checks if there is a frame received by DUT that can be read out.
         * @returns true if there is received frame, false otherwise.
         */
        virtual bool HasRxFrame() = 0;

        /**
         * @returns Value of REC (Receive Error counter)
         */
        virtual int GetRec() = 0;

        /**
         * @returns Value of TEC (Transmitt Error counter)
         */
        virtual int GetTec() = 0;

        /**
         * Forces value of REC within a DUT.
         * If DUT does not support direct modifications of REC (e.g. via test modes),
         * this function shall issue frames so that DUT will reach required REC value!
         * @param rec REC value to be set.
         */
        virtual void SetRec(int rec) = 0;

        /**
         * Forces value of TEC within a DUT.
         * If DUT does not support direct modifications of TEC (e.g. via test modes),
         * this function shall issue frames so that DUT will reach required TEC value!
         * @param tec TEC value to be set.
         */
        virtual void SetTec(int tec) = 0;

        /**
         * Sets DUT's fault confinement state.
         * If DUT does not support direct modifications of Fault confinement state,
         * this function shall issue frames so that DUT will reach required state!
         * @param fault_state state to be set
         */
        virtual void SetErrorState(FaultConfState fault_state) = 0;

        /**
         * @returns Fault confinement state of DUT.
         */
        virtual FaultConfState GetErrorState() = 0;

        /**
         * Configures PEX (Protocol exception).
         * @returns True if succefull, false otherwise (e.g. protocol exception not supported)
         */
        virtual bool ConfigureProtocolException(bool enable) = 0;

        /**
         * Configures One-shot mode.
         * @return True if succesfull, false otherwise (e.g. one shot mode not supported)
         */
        virtual bool ConfigureOneShot(bool enable) = 0;

        /**
         * Issues reintegration request to DUT.
         */
        virtual void SendReintegrationRequest() = 0;

        /**
         * Configures Restricted operation mode
         * @return True if succesfull, false otherwise (e.g. restricted operation mode not supported)
         */
        virtual bool ConfigureRestrictedOperation(bool enable) = 0;
};

#endif