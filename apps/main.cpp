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
#include <chrono>

#include "../can_lib/can.h"
#include "../can_lib/Frame.h"
#include "../can_lib/BitFrame.h"
#include "../can_lib/FrameFlags.h"
#include "../can_lib/BitTiming.h"

#include "../test_lib/test_lib.h"
#include "../test_lib/TestSequence.h"
#include "../test_lib/DriverItem.h"
#include "../test_lib/MonitorItem.h"

using namespace can;

int main()
{

    uint8_t data[64] =
    {
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
    };

    BitTiming nbt = BitTiming(2, 2, 2, 4, 1);
    BitTiming dbt = BitTiming(2, 2, 2, 1, 1);

    can::Frame frame = Frame();
    can::BitFrame bitFrame = BitFrame(
        FrameFlags(CanFd, Extended, DataFrame,
                   DontShift, ErrorActive),
        0, 32, &(data[0]), &nbt, &dbt);

    frame.Print();
    bitFrame.Print(true);
    bitFrame.InsertActiveErrorFrame(bitFrame.GetBitOf(1, Intermission));
    bitFrame.Print(true);

    std::chrono::nanoseconds clock_period(10);

    test_lib::TestSequence testSequence =
        test_lib::TestSequence(clock_period, bitFrame, test_lib::DRIVER_SEQUENCE);
    testSequence.PrintDrivenValues();
}