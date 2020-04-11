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
        virtual bool setFdStandardType(bool isIso) = 0;

        /**
         * 
         */
        virtual bool setCanVersion(CanVersion canVersion) = 0;

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
        virtual bool hasRxFrame() = 0;

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