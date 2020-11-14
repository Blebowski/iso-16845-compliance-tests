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

#include "can.h"
#include "Frame.h"

#ifndef DUT_INTERFACE
#define DUT_INTERFACE


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
        virtual void ConfigureBitTiming(BitTiming nominal_bit_timing,
                                        BitTiming data_bit_timing) = 0;

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
        virtual void SetErrorState(FaultConfinementState fault_state) = 0;

        /**
         * @returns Fault confinement state of DUT.
         */
        virtual FaultConfinementState GetErrorState() = 0;

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
};

#endif