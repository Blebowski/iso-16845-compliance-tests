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