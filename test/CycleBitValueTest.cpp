/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 29.5.2021
 * 
 * @brief Unit Test for "CycleBitValue" class
 *****************************************************************************/

#undef NDEBUG
#include <cassert>

#include "can_lib/CycleBitValue.h"

#include "can_lib/can.h"

using namespace can;


int main()
{
    CycleBitValue cv = CycleBitValue(nullptr, BitValue::Dominant);
    CycleBitValue cv2 = CycleBitValue(nullptr, BitValue::Recessive);

    assert(cv.bit_value() == BitValue::Dominant);
    assert(cv2.bit_value() == BitValue::Recessive);

    // Force and check it was forced
    cv.ForceValue(BitValue::Recessive);
    assert(cv.bit_value() == BitValue::Recessive);
    assert(cv.has_default_value() == false);

    // Release and check it was released.
    cv.ReleaseValue();
    assert(cv.has_default_value() == true);
    //assert(cv.bit_value() == BitValue::Dominant);

    return 0;
}