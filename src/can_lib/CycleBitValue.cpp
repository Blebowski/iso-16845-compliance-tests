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

can::CycleBitValue::CycleBitValue(TimeQuanta *parent):
                    parent_(parent)
{}

can::CycleBitValue::CycleBitValue(TimeQuanta *parent, BitValue bit_value):
    parent_(parent),
    has_default_value_(false),
    bit_value_(bit_value)
{}

void can::CycleBitValue::ForceValue(BitValue bit_value)
{
    has_default_value_ = false;
    bit_value_ = bit_value;
}

void can::CycleBitValue::ReleaseValue()
{
    has_default_value_ = true;
}