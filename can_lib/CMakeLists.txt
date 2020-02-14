
add_library(
    CAN_LIB

    can.h
    
    Bit.h
    CycleBit.h

    Frame.h
    BitFrame.h
    
    FrameFlags.h
    BitTiming.h

    Bit.cpp

    Frame.cpp
    BitFrame.cpp
    
    FrameFlags.cpp
    BitTiming.cpp
)

add_compile_options(CAN_LIB -g)
