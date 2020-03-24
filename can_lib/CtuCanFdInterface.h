/**
 * TODO: License
 */

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
        void setFdStandardType(bool isIso);
        void configureBitTiming(can::BitTiming nominalBitTiming,
                                can::BitTiming dataBitTiming);
        void sendFrame(can::Frame frame);
        can::Frame readFrame();
        int getRec();
        int getTec();
        void setErrorState(can::ErrorState errorState);
        can::ErrorState getErrorState();
};

};