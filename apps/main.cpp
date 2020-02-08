/*
 * TODO: License
 */

#include <iostream>

#include "../can_lib/can.h"
#include "../can_lib/CanFrame.h"
#include "../can_lib/CanBitFrame.h"

using namespace can;

int main()
{
    std::cout << "Hello world!" << std::endl;

    CanFrame canFrame = CanFrame();

    canFrame.print();
}