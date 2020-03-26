/*
 * TODO: License
 */

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