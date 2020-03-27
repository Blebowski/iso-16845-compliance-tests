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
    hasDefaultValue = true;
    // bitValue is effectively don't care, initialize it just for correctness!
    bitValue = RECESSIVE;
}


can::CycleBitValue::CycleBitValue(BitValue bitValue)
{
    hasDefaultValue = false;
    this->bitValue = bitValue;
}


void can::CycleBitValue::forceValue(BitValue bitValue)
{
    hasDefaultValue = false;
    this->bitValue = bitValue;
}

void can::CycleBitValue::releaseValue()
{
    hasDefaultValue = true;
}