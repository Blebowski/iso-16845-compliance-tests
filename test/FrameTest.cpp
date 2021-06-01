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
 * @brief Unit Test for "Frame" class
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
    Frame f1 = Frame();
    f1.Randomize();
    f1.Print();

    // Check valid data lenghts according to ISO standard
    assert(f1.dlc() >= 0x0 && f1.dlc() <= 0xF);
    assert(f1.data_length() == 0 || f1.data_length() == 1 ||
           f1.data_length() == 2 || f1.data_length() == 3 ||
           f1.data_length() == 4 || f1.data_length() == 5 ||
           f1.data_length() == 6 || f1.data_length() == 7 ||
           f1.data_length() == 8 || f1.data_length() == 12 ||
           f1.data_length() == 16 || f1.data_length() == 20 ||
           f1.data_length() == 24 || f1.data_length() == 32 ||
           f1.data_length() == 48 || f1.data_length() == 64);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Nothing should be randomized
    ///////////////////////////////////////////////////////////////////////////////////////////////
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    Frame f2 = Frame(FrameFlags(), 4, 100, data);
    f2.Randomize();
    f2.Print();

    assert(f2.identifier() == 100 &&
           f2.dlc() == 4 &&
           f2.data(0) == 0xAA &&
           f2.data(1) == 0xBB &&
           f2.data(2) == 0xCC &&
           f2.data(3) == 0xDD);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Randomize data
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Frame f3 = Frame(FrameFlags(), 8, 256);
    f3.Randomize();
    f3.Print();

    assert(f3.identifier() == 256 &&
           f3.dlc() == 8);
    for (int i = 8; i < 64; i++)
        assert(f3.data(i) == 0x0);

}

void test_operator_overload()
{
    FrameFlags ff1 = FrameFlags(FrameType::CanFd);
    FrameFlags ff2 = FrameFlags(FrameType::Can2_0);
    uint8_t data_a[4] = {0x00, 0x01, 0x02, 0x03};
    uint8_t data_b[4] = {0x00, 0x01, 0x02, 0x0C};

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Check all equal
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Frame f1 = Frame(ff1, 4, 100, data_a);
    Frame f2 = Frame(ff1, 4, 100, data_a);
    assert(f1 == f2);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Check Flags are not equal
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Frame f3 = Frame(ff1, 4, 100, data_a);
    Frame f4 = Frame(ff2, 4, 100, data_a);
    assert(f3 != f4);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Check DLCs are not equal
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Frame f5 = Frame(ff1, 4, 100, data_a);
    Frame f6 = Frame(ff1, 3, 100, data_a);
    assert(f5 != f6);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Check Identifiers are not equal
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Frame f7 = Frame(ff1, 4, 80, data_a);
    Frame f8 = Frame(ff1, 4, 100, data_a);
    assert(f7 != f8);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Check Data bytes are not equal
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Frame f9 = Frame(ff1, 4, 80, data_a);
    Frame f10 = Frame(ff1, 4, 100, data_b);
    assert(f9 != f10);
}


int main()
{
    test_randomization();
    test_operator_overload();
}