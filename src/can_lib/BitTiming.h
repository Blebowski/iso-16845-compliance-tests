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