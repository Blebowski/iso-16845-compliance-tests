/**
 * TODO: License
 */

#include "can.h"
#include "Frame.h"

class can::DutInterface
{
    public:

        /**
         * 
         */
        virtual void enable();

        /**
         *
         */
        virtual void disable();

        /**
         * 
         */
        virtual void configureBitTiming(BitTiming nominalBitTiming,
                                        BitTiming dataBitTiming);

        /**
         * 
         */
        virtual void sendFrame(Frame frame);

        /**
         * 
         */
        virtual Frame readFrame();

        /**
         * 
         */
        virtual int getRec();
        
        /**
         * 
         */
        virtual int getTec();

        /**
         * 
         */
        virtual void setErrorState(ErrorState errorState);

        /**
         * 
         */
        virtual ErrorState getErrorState();
}