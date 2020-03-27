
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
    COMPLIANCE_TESTS SHARED

    TestDemo.cpp
)

add_compile_options(COMPLIANCE_TESTS -g -fPIC -Wall -O0)

target_link_libraries(COMPLIANCE_TESTS PUBLIC VPI_LIB)