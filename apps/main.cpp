/*
 * TODO: License
 */

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
        FrameFlags(CAN_FD, EXTENDED_IDENTIFIER, DATA_FRAME,
                   BIT_RATE_DONT_SHIFT, ESI_ERROR_ACTIVE),
        0, 32, &(data[0]), &nbt, &dbt);

    frame.print();
    bitFrame.print(true);
    bitFrame.insertActiveErrorFrame(bitFrame.getBitOf(1, BIT_TYPE_INTERMISSION));
    bitFrame.print(true);

    std::chrono::nanoseconds clock_period(10);

    test_lib::TestSequence testSequence =
        test_lib::TestSequence(clock_period, bitFrame, test_lib::DRIVER_SEQUENCE);
    testSequence.printDrivenValues();
}