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

#include <assert.h>

#include "can.h"
#include "BitTiming.h"


can::BitTiming::BitTiming(size_t prop, size_t ph1, size_t ph2, size_t brp, size_t sjw)
{
    prop_ = prop;
    ph1_ = ph1;
    ph2_ = ph2;
    sjw_ = sjw;
    brp_ = brp;

    // SJW can't be larger than any of TSEG1 or TSEG2. Don't account any IPT
    // as this model is ideal implementation
    assert(sjw <= prop + ph1 + 1);
    assert(sjw <= ph2);

    assert(sjw <= prop + ph1 + 1);
    assert(sjw <= ph2);
}

can::BitTiming::BitTiming(){}


void can::BitTiming::Print()
{
    std::cout << "BRP:  " << brp_ << std::endl;
    std::cout << "PROP: " << prop_ << std::endl;
    std::cout << "PH1:  " << ph1_ << std::endl;
    std::cout << "PH2:  " << ph2_ << std::endl;
    std::cout << "SJW:  " << sjw_ << std::endl;
}


size_t can::BitTiming::GetBitLengthTimeQuanta()
{
    return prop_ + ph1_ + ph2_ + 1;
}

size_t can::BitTiming::GetBitLengthCycles()
{
    return GetBitLengthTimeQuanta() * brp_;
}