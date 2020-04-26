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

class can::CtuCanFdInterface : public can::DutInterface
{
    /**
     * All functions implement virtual interface "DutInterface" which allows
     * writing tests in DUT independent manner! All configuration of DUT shall
     * be executed via functions of this interface, not by manuall access to
     * DUT!
     */
    public:
        void enable();
        void disable();
        void reset();
        bool setFdStandardType(bool isIso);
        bool setCanVersion(CanVersion canVersion);
        void configureBitTiming(can::BitTiming nominalBitTiming,
                                can::BitTiming dataBitTiming);
        void sendFrame(can::Frame frame);
        can::Frame readFrame();
        bool hasRxFrame();
        int getRec();
        int getTec();
        void setRec(int rec);
        void setTec(int tec);
        void setErrorState(can::ErrorState errorState);
        can::ErrorState getErrorState();
};

};