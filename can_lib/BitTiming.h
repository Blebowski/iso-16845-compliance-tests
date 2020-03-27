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

class can::BitTiming
{
    public:
        unsigned int prop;
        unsigned int ph1;
        unsigned int ph2;
        unsigned int brp;
        unsigned int sjw;

        BitTiming(unsigned int prop, unsigned int ph1, unsigned int ph2,
                  unsigned int brp, unsigned int sjw);
        BitTiming();

        void print();
        
};

#endif