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
        unsigned int prop_;
        unsigned int ph1_;
        unsigned int ph2_;
        unsigned int brp_;
        unsigned int sjw_;

        BitTiming(unsigned int prop, unsigned int ph1, unsigned int ph2,
                  unsigned int brp, unsigned int sjw);
        BitTiming();

        void Print();
        
};

#endif