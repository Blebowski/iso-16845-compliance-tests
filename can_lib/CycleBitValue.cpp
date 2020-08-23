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
#include "CycleBitValue.h"

can::CycleBitValue::CycleBitValue()
{
    has_default_value_ = true;
    // bitValue is effectively don't care, initialize it just for correctness!
    bit_value_ = BitValue::Recessive;
}


can::CycleBitValue::CycleBitValue(BitValue bit_value)
{
    has_default_value_ = false;
    bit_value_ = bit_value;
}


void can::CycleBitValue::ForceValue(BitValue bit_value)
{
    has_default_value_ = false;
    bit_value_ = bit_value;
}

void can::CycleBitValue::ReleaseValue()
{
    has_default_value_ = true;
}