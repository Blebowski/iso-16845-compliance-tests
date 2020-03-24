/*
 * TODO: License
 */

#include <assert.h>

#include "can.h"
#include "BitTiming.h"


can::BitTiming::BitTiming(unsigned int prop, unsigned int ph1, unsigned int ph2,
                          unsigned int brp, unsigned int sjw)
{
    this->prop = prop;
    this->ph1 = ph1;
    this->ph2 = ph2;
    this->sjw = sjw;
    this->brp = brp;

    // SJW can't be larger than any of TSEG1 or TSEG2. Don't account any IPT
    // as this model is ideal implementation
    assert(sjw <= prop + ph1 + 1);
    assert(sjw <= ph2);

    assert(sjw <= prop + ph1 + 1);
    assert(sjw <= ph2);
}

can::BitTiming::BitTiming(){};


void can::BitTiming::print()
{
    std::cout << "BRP:  " << this->brp << std::endl;
    std::cout << "PROP: " << this->prop << std::endl;
    std::cout << "PH1:  " << this->ph1 << std::endl;
    std::cout << "PH2:  " << this->ph2 << std::endl;
    std::cout << "SJW:  " << this->sjw << std::endl;
}