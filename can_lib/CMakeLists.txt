
add_library(
    CAN_LIB SHARED

    # CAN namespace
    can.h
    
    # Headers
    Bit.h
    TimeQuanta.h
    CycleBitValue.h

    Frame.h
    BitFrame.h
    
    FrameFlags.h
    BitTiming.h

    # Implementation
    Bit.cpp
    TimeQuanta.cpp
    CycleBitValue.cpp

    Frame.cpp
    BitFrame.cpp
    
    FrameFlags.cpp
    BitTiming.cpp
)

add_compile_options(CAN_LIB -g -fPIC -Wall -O0)
