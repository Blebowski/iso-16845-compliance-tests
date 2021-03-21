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
    CAN_LIB OBJECT

    # CAN namespace
    can.h
    can.cpp
    
    # Headers
    Bit.h
    TimeQuanta.h
    CycleBitValue.h

    Frame.h
    BitFrame.h
    
    FrameFlags.h
    BitTiming.h

    ../../../../driver/ctucanfd_frame.h
    ../../../../driver/ctucanfd_regs.h

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

target_compile_definitions(CAN_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)
