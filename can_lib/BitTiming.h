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

#ifndef BIT_TIMING
#define BIT_TIMING


/**
 * @class BitTiming
 * @namespace can
 * 
 * Class representing bit time setting on CAN bus. Single data bit rate is
 * expressed by a class.
 * 
 */
class can::BitTiming
{
    public:
        size_t prop_ = 2;
        size_t ph1_ = 2;
        size_t ph2_ = 2;
        size_t brp_ = 2;
        size_t sjw_ = 2;

        BitTiming(size_t prop, size_t ph1, size_t ph2, size_t brp, size_t sjw);
        BitTiming();

        void Print();
        
        /**
         * @returns Overall bit length in Time quantas.
         */
        size_t GetBitLengthTimeQuanta();

        /**
         * @returns Overall bit length in clock cycles.
         */
        size_t GetBitLengthCycles();
};

#endif