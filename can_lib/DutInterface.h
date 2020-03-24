/**
 * TODO: License
 */

#include "can.h"
#include "Frame.h"

#ifndef DUT_INTERFACE
#define DUT_INTERFACE


class can::DutInterface
{
    public:

        /**
         * 
         */
        virtual void enable() = 0;

        /**
         *
         */
        virtual void disable() = 0;

        /**
         *
         */
        virtual void reset() = 0; 

        /**
         * 
         */
        virtual void setFdStandardType(bool isIso) = 0;

        /**
         * 
         */
        virtual void configureBitTiming(BitTiming nominalBitTiming,
                                        BitTiming dataBitTiming) = 0;

        /**
         * 
         */
        virtual void sendFrame(Frame frame) = 0;

        /**
         * 
         */
        virtual Frame readFrame() = 0;

        /**
         * 
         */
        virtual int getRec() = 0;
        
        /**
         * 
         */
        virtual int getTec() = 0;

        /**
         * 
         */
        virtual void setErrorState(ErrorState errorState) = 0;

        /**
         * 
         */
        virtual ErrorState getErrorState() = 0;
};

#endif