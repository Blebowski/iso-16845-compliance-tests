
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
    TEST_LIB SHARED

    # CAN namespace
    test_lib.h
    
    # Headers
    DriverItem.h
    MonitorItem.h
    TestSequence.h
    TestLoader.h
    TestBase.h
    
    # Implementation
    DriverItem.cpp
    MonitorItem.cpp
    TestSequence.cpp
    TestLoader.cpp
    TestBase.cpp
)

add_compile_options(TEST_LIB -g -fPIC -Wall -O0)
