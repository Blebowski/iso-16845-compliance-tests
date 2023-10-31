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
    assert(!(ff1.is_fdf() == FrameKind::CanFd && ff1.is_rtr() == RtrFlag::Rtr));
    assert(!(ff1.is_fdf() == FrameKind::CanFd && ff1.is_esi() == EsiFlag::ErrPas));

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Nothing should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff2 = FrameFlags(FrameKind::Can20, IdentKind::Base, RtrFlag::Data,
                                BrsFlag::NoShift, EsiFlag::ErrAct);
    ff2.Randomize();
    assert(ff2.is_fdf() == FrameKind::Can20 &&
           ff2.is_ide() == IdentKind::Base &&
           ff2.is_rtr() == RtrFlag::Data &&
           ff2.is_brs() == BrsFlag::NoShift &&
           ff2.is_esi() == EsiFlag::ErrAct);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BRS should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff3 = FrameFlags(FrameKind::CanFd, IdentKind::Base, RtrFlag::Data,
                                EsiFlag::ErrPas);
    ff3.Randomize();
    assert(ff3.is_fdf() == FrameKind::CanFd &&
           ff3.is_ide() == IdentKind::Base &&
           ff3.is_rtr() == RtrFlag::Data &&
           ff3.is_esi() == EsiFlag::ErrPas);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // IDE should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff4 = FrameFlags(FrameKind::CanFd, RtrFlag::Data, BrsFlag::DoShift,
                                EsiFlag::ErrAct);
    ff4.Randomize();
    assert(ff4.is_fdf() == FrameKind::CanFd &&
           ff4.is_rtr() == RtrFlag::Data &&
           ff4.is_brs() == BrsFlag::DoShift &&
           ff4.is_esi() == EsiFlag::ErrAct);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BRS and ESI randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff5 = FrameFlags(FrameKind::CanFd, IdentKind::Base, RtrFlag::Rtr);

    ff5.Randomize();
    assert(ff5.is_fdf() == FrameKind::CanFd &&
           ff5.is_rtr() == RtrFlag::Data && // RTR flag should be ignored in FDF frames
           ff5.is_ide() == IdentKind::Base);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes RTR flag, BRS and ESI
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff6 = FrameFlags(FrameKind::CanFd, IdentKind::Base);

    ff6.Randomize();
    assert(ff6.is_fdf() == FrameKind::CanFd &&
           ff6.is_ide() == IdentKind::Base);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS, ESI
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff7 = FrameFlags(FrameKind::Can20, RtrFlag::Rtr);

    ff7.Randomize();
    assert(ff7.is_fdf() == FrameKind::Can20 &&
           ff7.is_rtr() == RtrFlag::Rtr);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff8 = FrameFlags(FrameKind::CanFd, RtrFlag::Data, EsiFlag::ErrAct);

    ff8.Randomize();
    assert(ff8.is_fdf() == FrameKind::CanFd &&
           ff8.is_rtr() == RtrFlag::Data &&
           ff8.is_esi() == EsiFlag::ErrAct);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS, ESI, RTR.
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff9 = FrameFlags(FrameKind::CanFd);

    ff9.Randomize();
    assert(ff9.is_fdf() == FrameKind::CanFd);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes FDF, BRS, ESI, RTR
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff10 = FrameFlags(IdentKind::Ext);

    ff10.Randomize();
    assert(ff10.is_ide() == IdentKind::Ext);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes ESI, IDE, RTR
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff11 = FrameFlags(FrameKind::Can20, BrsFlag::DoShift);

    ff11.Randomize();
    assert(ff11.is_fdf() == FrameKind::Can20 &&
           ff11.is_brs() == BrsFlag::NoShift); // Should not shift, is CAN 2.0 frame!

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes RTR and IDE
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff12 = FrameFlags(FrameKind::Can20, BrsFlag::NoShift, EsiFlag::ErrAct);

    ff12.Randomize();
    assert(ff12.is_fdf() == FrameKind::Can20 &&
           ff12.is_brs() == BrsFlag::NoShift &&
           ff12.is_esi() == EsiFlag::ErrAct);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes BRS, RTR, IDE
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff13 = FrameFlags(FrameKind::CanFd, EsiFlag::ErrAct);

    assert(ff13.is_fdf() == FrameKind::CanFd &&
           ff13.is_rtr() == RtrFlag::Data && // RTR frames do not exist in CAN FD!
           ff13.is_esi() == EsiFlag::ErrAct);

    //assert(false);
}


int main()
{
    test_randomization();

    // Check operator overload
    FrameFlags ff1 = FrameFlags(FrameKind::Can20, IdentKind::Base, RtrFlag::Data,
                                BrsFlag::NoShift, EsiFlag::ErrAct);
    FrameFlags ff2 = FrameFlags(FrameKind::Can20, IdentKind::Base, RtrFlag::Data,
                                BrsFlag::NoShift, EsiFlag::ErrAct);
    assert(ff1 == ff2);

    FrameFlags ff3 = FrameFlags(FrameKind::CanFd, IdentKind::Base, RtrFlag::Data,
                                BrsFlag::NoShift, EsiFlag::ErrAct);
    FrameFlags ff4 = FrameFlags(FrameKind::Can20, IdentKind::Base, RtrFlag::Data,
                                BrsFlag::NoShift, EsiFlag::ErrPas);
    assert(ff3 != ff4);
}