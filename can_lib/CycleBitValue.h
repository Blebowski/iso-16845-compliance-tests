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

#include <iostream>

#include "can.h"

#ifndef CYCLE_BIT_VALUE
#define CYCLE_BIT_VALUE

/**
 * @class CycleBitValue
 * @namespace can
 * 
 * Represents value of single clock cycle within a time quanta.
 */
class can::CycleBitValue
{
    public:

        /**
         * Default value for given cycle.
         */
        CycleBitValue(TimeQuanta *parent);

        /**
         * Forced value for given cycle.
         */
        CycleBitValue(TimeQuanta *parent, BitValue bit_value);

        /**
         * Forces value within a cycle
         * @param bit_value Value to force
         */
        void ForceValue(BitValue bit_value);

        /**
         * Releases value within given cycle (returns to default value)
         */
        void ReleaseValue();

        /* Default value from CanBit should be taken */
        bool has_default_value_;

        /* If has_default_value_ = false, then cycle has this value */
        BitValue bit_value_;
    
    protected:
        /* Time quanta which contains this cycle */
        TimeQuanta *parent_;
};

#endif