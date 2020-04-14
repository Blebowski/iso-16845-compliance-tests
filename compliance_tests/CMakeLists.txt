
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
    TestIso_7_1_1.cpp
    TestIso_7_1_4.cpp
    TestIso_7_1_5.cpp
    TestIso_7_1_8.cpp
    TestIso_7_1_9.cpp
    TestIso_7_1_12.cpp

    TestIso_7_2_1.cpp
    TestIso_7_2_6.cpp
    TestIso_7_2_7.cpp
    TestIso_7_2_9.cpp
    TestIso_7_2_10.cpp
    TestIso_7_2_11.cpp

    TestIso_7_3_1.cpp
    TestIso_7_3_2.cpp
    TestIso_7_3_3.cpp
    TestIso_7_3_4.cpp

    TestIso_7_4_1.cpp
    TestIso_7_4_2.cpp
)

add_compile_options(COMPLIANCE_TESTS -g -fPIC -Wall -O0)

target_link_libraries(COMPLIANCE_TESTS PUBLIC VPI_LIB)