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
    assert(ff1.randomize_brs_ &&
           ff1.randomize_esi_ &&
           ff1.randomize_fdf_ &&
           ff1.randomize_ide_ &&
           ff1.randomize_rtr_);
    ff1.Randomize();
    
    // Check there are no invalid configs
    assert(!(ff1.is_fdf_ == FrameType::CanFd && ff1.is_rtr_ == RtrFlag::RtrFrame));
    assert(!(ff1.is_fdf_ == FrameType::CanFd && ff1.is_esi_ == EsiFlag::ErrorPassive));

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Nothing should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff2 = FrameFlags(FrameType::Can2_0, IdentifierType::Base, RtrFlag::DataFrame,
                                BrsFlag::DontShift, EsiFlag::ErrorActive);
    assert(!ff2.randomize_brs_ &&
           !ff2.randomize_esi_ &&
           !ff2.randomize_fdf_ &&
           !ff2.randomize_ide_ &&
           !ff2.randomize_rtr_);
    
    ff2.Randomize();
    assert(ff2.is_fdf_ == FrameType::Can2_0 &&
           ff2.is_ide_ == IdentifierType::Base &&
           ff2.is_rtr_ == RtrFlag::DataFrame &&
           ff2.is_brs_ == BrsFlag::DontShift &&
           ff2.is_esi_ == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BRS should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff3 = FrameFlags(FrameType::CanFd, IdentifierType::Base, RtrFlag::DataFrame,
                                EsiFlag::ErrorPassive);
    assert(ff3.randomize_brs_ &&
           !ff3.randomize_esi_ &&
           !ff3.randomize_fdf_ &&
           !ff3.randomize_ide_ &&
           !ff3.randomize_rtr_);

    ff3.Randomize();
    assert(ff3.is_fdf_ == FrameType::CanFd &&
           ff3.is_ide_ == IdentifierType::Base &&
           ff3.is_rtr_ == RtrFlag::DataFrame &&
           ff3.is_esi_ == EsiFlag::ErrorPassive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // IDE should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff4 = FrameFlags(FrameType::CanFd, RtrFlag::DataFrame, BrsFlag::Shift,
                                EsiFlag::ErrorActive);
    assert(!ff4.randomize_brs_ &&
           !ff4.randomize_esi_ &&
           !ff4.randomize_fdf_ &&
           ff4.randomize_ide_ &&
           !ff4.randomize_rtr_);

    ff4.Randomize();
    assert(ff4.is_fdf_ == FrameType::CanFd &&
           ff4.is_rtr_ == RtrFlag::DataFrame &&
           ff4.is_brs_ == BrsFlag::Shift &&
           ff4.is_esi_ == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BRS and ESI randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff5 = FrameFlags(FrameType::CanFd, IdentifierType::Base, RtrFlag::RtrFrame);
    assert(ff5.randomize_brs_ &&
           ff5.randomize_esi_ &&
           !ff5.randomize_fdf_ &&
           !ff5.randomize_ide_ &&
           !ff5.randomize_rtr_);

    ff5.Randomize();
    assert(ff5.is_fdf_ == FrameType::CanFd &&
           ff5.is_rtr_ == RtrFlag::DataFrame && // RTR flag should be ignored in FDF frames
           ff5.is_ide_ == IdentifierType::Base);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes RTR flag, BRS and ESI
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff6 = FrameFlags(FrameType::CanFd, IdentifierType::Base);
    assert(ff6.randomize_brs_ &&
           ff6.randomize_esi_ &&
           !ff6.randomize_fdf_ &&
           !ff6.randomize_ide_ &&
           ff6.randomize_rtr_);

    ff6.Randomize();
    assert(ff6.is_fdf_ == FrameType::CanFd &&
           ff6.is_ide_ == IdentifierType::Base);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS, ESI
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff7 = FrameFlags(FrameType::Can2_0, RtrFlag::RtrFrame);
    assert(ff7.randomize_brs_ &&
           ff7.randomize_esi_ &&
           !ff7.randomize_fdf_ &&
           ff7.randomize_ide_ &&
           !ff7.randomize_rtr_);

    ff7.Randomize();
    assert(ff7.is_fdf_ == FrameType::Can2_0 &&
           ff7.is_rtr_ == RtrFlag::RtrFrame);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff8 = FrameFlags(FrameType::CanFd, RtrFlag::DataFrame, EsiFlag::ErrorActive);
    assert(ff8.randomize_brs_ &&
           !ff8.randomize_esi_ &&
           !ff8.randomize_fdf_ &&
           ff8.randomize_ide_ &&
           !ff8.randomize_rtr_);

    ff8.Randomize();
    assert(ff8.is_fdf_ == FrameType::CanFd &&
           ff8.is_rtr_ == RtrFlag::DataFrame &&
           ff8.is_esi_ == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes IDE, BRS, ESI, RTR.
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff9 = FrameFlags(FrameType::CanFd);
    assert(ff9.randomize_brs_ &&
           ff9.randomize_esi_ &&
           !ff9.randomize_fdf_ &&
           ff9.randomize_ide_ &&
           ff9.randomize_rtr_); 
    
    ff9.Randomize();
    assert(ff9.is_fdf_ == FrameType::CanFd);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes FDF, BRS, ESI, RTR
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff10 = FrameFlags(IdentifierType::Extended);
    assert(ff10.randomize_brs_ &&
           ff10.randomize_esi_ &&
           ff10.randomize_fdf_ &&
           !ff10.randomize_ide_ &&
           ff10.randomize_rtr_); 

    ff10.Randomize();
    assert(ff10.is_ide_ == IdentifierType::Extended);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes ESI, IDE, RTR
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff11 = FrameFlags(FrameType::Can2_0, BrsFlag::Shift);
    assert(!ff11.randomize_brs_ &&
           ff11.randomize_esi_ &&
           !ff11.randomize_fdf_ &&
           ff11.randomize_ide_ &&
           ff11.randomize_rtr_); 

    ff11.Randomize();
    assert(ff11.is_fdf_ == FrameType::Can2_0 &&
           ff11.is_brs_ == BrsFlag::DontShift); // Should not shift, is CAN 2.0 frame!

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes RTR and IDE
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff12 = FrameFlags(FrameType::Can2_0, BrsFlag::DontShift, EsiFlag::ErrorActive);
    assert(!ff12.randomize_brs_ &&
           !ff12.randomize_esi_ &&
           !ff12.randomize_fdf_ &&
           ff12.randomize_ide_ &&
           ff12.randomize_rtr_); 

    ff12.Randomize();
    assert(ff12.is_fdf_ == FrameType::Can2_0 &&
           ff12.is_brs_ == BrsFlag::DontShift &&
           ff12.is_esi_ == EsiFlag::ErrorActive);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomizes BRS, RTR, IDE
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FrameFlags ff13 = FrameFlags(FrameType::CanFd, EsiFlag::ErrorActive);
    assert(ff13.randomize_brs_ &&
           !ff13.randomize_esi_ &&
           !ff13.randomize_fdf_ &&
           ff13.randomize_ide_ &&
           ff13.randomize_rtr_);

    assert(ff13.is_fdf_ == FrameType::CanFd &&
           ff13.is_rtr_ == RtrFlag::DataFrame && // RTR frames do not exist in CAN FD!
           ff13.is_esi_ == EsiFlag::ErrorActive);

    //assert(false);
}


int main()
{
    test_randomization();

}