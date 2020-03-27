###############################################################################
# 
# Copyright (C) Ondrej Ille - All Rights Reserved
#  
# Copying, publishing, distributing of this file is stricly prohibited unless
# previously aggreed with author of this text.
# 
# Author: Ondrej Ille, <ondrej.ille@gmail.com>
# Date: 27.3.2020
# 
###############################################################################

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

    ../../../../driver/ctu_can_fd_frame.h
    ../../../../driver/ctu_can_fd_regs.h

    DutInterface.h
    CtuCanFdInterface.h

    # Implementation
    Bit.cpp
    TimeQuanta.cpp
    CycleBitValue.cpp

    Frame.cpp
    BitFrame.cpp
    
    FrameFlags.cpp
    BitTiming.cpp

    CtuCanFdInterface.cpp

)

add_compile_options(CAN_LIB -g -fPIC -Wall -O0)
target_compile_definitions(CAN_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)
