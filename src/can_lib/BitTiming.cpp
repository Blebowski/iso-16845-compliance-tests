/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 *
 *****************************************************************************/

#include <assert.h>

#include "can.h"
#include "BitTiming.h"


can::BitTiming::BitTiming(size_t prop, size_t ph1, size_t ph2, size_t brp, size_t sjw):
    prop_(prop),
    ph1_(ph1),
    ph2_(ph2),
    brp_(brp),
    sjw_(sjw)
{
    // SJW can't be larger than any of TSEG1 or TSEG2. Don't account any IPT
    // as this model is ideal implementation
    assert(sjw <= prop + ph1 + 1);
    assert(sjw <= ph2);

    assert(sjw <= prop + ph1 + 1);
    assert(sjw <= ph2);
}

can::BitTiming::BitTiming()
{}


void can::BitTiming::Print()
{
    std::cout << "BRP:  " << brp_ << '\n';
    std::cout << "PROP: " << prop_ << '\n';
    std::cout << "PH1:  " << ph1_ << '\n';
    std::cout << "PH2:  " << ph2_ << '\n';
    std::cout << "SJW:  " << sjw_ << std::endl;
}


size_t can::BitTiming::GetBitLenTQ()
{
    return prop_ + ph1_ + ph2_ + 1;
}

size_t can::BitTiming::GetBitLenCycles()
{
    return GetBitLenTQ() * brp_;
}