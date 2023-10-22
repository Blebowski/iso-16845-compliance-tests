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
 * @brief Unit Test for "FrameFlags" class
 *****************************************************************************/

#undef NDEBUG
#include <cassert>

#include "../src/can_lib/Frame.h"
#include "../src/can_lib/FrameFlags.h"

#include "../src/can_lib/can.h"

using namespace can;


void test_randomization()
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Everything should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff1 = FrameFlags();
    ff1.Randomize();

    // Check there are no invalid configs
    assert(!(ff1.is_fdf() == FrameType::CanFd && ff1.is_rtr() == RtrFlag::RtrFrame));
    assert(!(ff1.is_fdf() == FrameType::CanFd && ff1.is_esi() == EsiFlag::ErrorPassive));

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Nothing should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff2 = FrameFlags(FrameType::Can2_0, IdentifierType::Base, RtrFlag::DataFrame,
                                BrsFlag::DontShift, EsiFlag::ErrorActive);
    ff2.Randomize();
    assert(ff2.is_fdf() == FrameType::Can2_0 &&
           ff2.is_ide() == IdentifierType::Base &&
           ff2.is_rtr() == RtrFlag::DataFrame &&
           ff2.is_brs() == BrsFlag::DontShift &&
           ff2.is_esi() == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BRS should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff3 = FrameFlags(FrameType::CanFd, IdentifierType::Base, RtrFlag::DataFrame,
                                EsiFlag::ErrorPassive);
    ff3.Randomize();
    assert(ff3.is_fdf() == FrameType::CanFd &&
           ff3.is_ide() == IdentifierType::Base &&
           ff3.is_rtr() == RtrFlag::DataFrame &&
           ff3.is_esi() == EsiFlag::ErrorPassive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // IDE should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff4 = FrameFlags(FrameType::CanFd, RtrFlag::DataFrame, BrsFlag::Shift,
                                EsiFlag::ErrorActive);
    ff4.Randomize();
    assert(ff4.is_fdf() == FrameType::CanFd &&
           ff4.is_rtr() == RtrFlag::DataFrame &&
           ff4.is_brs() == BrsFlag::Shift &&
           ff4.is_esi() == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BRS and ESI randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff5 = FrameFlags(FrameType::CanFd, IdentifierType::Base, RtrFlag::RtrFrame);

    ff5.Randomize();
    assert(ff5.is_fdf() == FrameType::CanFd &&
           ff5.is_rtr() == RtrFlag::DataFrame && // RTR flag should be ignored in FDF frames
           ff5.is_ide() == IdentifierType::Base);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes RTR flag, BRS and ESI
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff6 = FrameFlags(FrameType::CanFd, IdentifierType::Base);

    ff6.Randomize();
    assert(ff6.is_fdf() == FrameType::CanFd &&
           ff6.is_ide() == IdentifierType::Base);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS, ESI
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff7 = FrameFlags(FrameType::Can2_0, RtrFlag::RtrFrame);

    ff7.Randomize();
    assert(ff7.is_fdf() == FrameType::Can2_0 &&
           ff7.is_rtr() == RtrFlag::RtrFrame);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff8 = FrameFlags(FrameType::CanFd, RtrFlag::DataFrame, EsiFlag::ErrorActive);

    ff8.Randomize();
    assert(ff8.is_fdf() == FrameType::CanFd &&
           ff8.is_rtr() == RtrFlag::DataFrame &&
           ff8.is_esi() == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS, ESI, RTR.
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff9 = FrameFlags(FrameType::CanFd);

    ff9.Randomize();
    assert(ff9.is_fdf() == FrameType::CanFd);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes FDF, BRS, ESI, RTR
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff10 = FrameFlags(IdentifierType::Extended);

    ff10.Randomize();
    assert(ff10.is_ide() == IdentifierType::Extended);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes ESI, IDE, RTR
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff11 = FrameFlags(FrameType::Can2_0, BrsFlag::Shift);

    ff11.Randomize();
    assert(ff11.is_fdf() == FrameType::Can2_0 &&
           ff11.is_brs() == BrsFlag::DontShift); // Should not shift, is CAN 2.0 frame!

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes RTR and IDE
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff12 = FrameFlags(FrameType::Can2_0, BrsFlag::DontShift, EsiFlag::ErrorActive);

    ff12.Randomize();
    assert(ff12.is_fdf() == FrameType::Can2_0 &&
           ff12.is_brs() == BrsFlag::DontShift &&
           ff12.is_esi() == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes BRS, RTR, IDE
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff13 = FrameFlags(FrameType::CanFd, EsiFlag::ErrorActive);

    assert(ff13.is_fdf() == FrameType::CanFd &&
           ff13.is_rtr() == RtrFlag::DataFrame && // RTR frames do not exist in CAN FD!
           ff13.is_esi() == EsiFlag::ErrorActive);

    //assert(false);
}


int main()
{
    test_randomization();

    // Check operator overload
    FrameFlags ff1 = FrameFlags(FrameType::Can2_0, IdentifierType::Base, RtrFlag::DataFrame,
                                BrsFlag::DontShift, EsiFlag::ErrorActive);
    FrameFlags ff2 = FrameFlags(FrameType::Can2_0, IdentifierType::Base, RtrFlag::DataFrame,
                                BrsFlag::DontShift, EsiFlag::ErrorActive);
    assert(ff1 == ff2);

    FrameFlags ff3 = FrameFlags(FrameType::CanFd, IdentifierType::Base, RtrFlag::DataFrame,
                                BrsFlag::DontShift, EsiFlag::ErrorActive);
    FrameFlags ff4 = FrameFlags(FrameType::Can2_0, IdentifierType::Base, RtrFlag::DataFrame,
                                BrsFlag::DontShift, EsiFlag::ErrorPassive);
    assert(ff3 != ff4);
}