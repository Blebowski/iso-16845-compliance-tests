#ifndef CYCLE_BIT_VALUE_H
#define CYCLE_BIT_VALUE_H
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

#include <iostream>

#include "can.h"

/**
 * @class CycleBitValue
 * @namespace can
 *
 * Represents value of single clock cycle within a time quanta.
 */
class can::Cycle
{
    public:

        /**
         * Default value for given cycle.
         */
        Cycle(TimeQuanta *parent);

        /**
         * Forced value for given cycle.
         */
        Cycle(TimeQuanta *parent, BitVal val);

        /**
         * Forces value within a cycle
         * @param val Value to force
         */
        void ForceVal(BitVal val);

        /**
         * Releases value within given cycle (returns to default value)
         */
        void ReleaseVal();

        // Getters
        inline bool has_def_val() const {
            return has_def_val_;
        };

        inline BitVal bit_val() const {
            return val_;
        };

    protected:
        /* Time quanta which contains this cycle */
        TimeQuanta *parent_;

        /* Default value from CanBit should be taken */
        bool has_def_val_ = true;

        /* If has_default_value_ = false, then cycle has this value */
        BitVal val_ = BitVal::Recessive;
};

#endif