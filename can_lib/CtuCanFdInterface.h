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
    };

};