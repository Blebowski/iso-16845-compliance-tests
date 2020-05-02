
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
    TestIso_7_4_4.cpp
    TestIso_7_4_5.cpp

    TestIso_7_6_1.cpp
    TestIso_7_6_2.cpp
    TestIso_7_6_3.cpp
    TestIso_7_6_4.cpp
    TestIso_7_6_5.cpp
    TestIso_7_6_6.cpp
    TestIso_7_6_7.cpp
    TestIso_7_6_8.cpp
    TestIso_7_6_11.cpp
    TestIso_7_6_12.cpp
    TestIso_7_6_13.cpp
    TestIso_7_6_14.cpp
    TestIso_7_6_15.cpp
    TestIso_7_6_16.cpp
    TestIso_7_6_17.cpp
    TestIso_7_6_18.cpp
    TestIso_7_6_19.cpp
    TestIso_7_6_20.cpp

    TestIso_7_7_1.cpp
    TestIso_7_7_9_1.cpp
)

add_compile_options(COMPLIANCE_TESTS -g -fPIC -Wall -O0)

target_link_libraries(COMPLIANCE_TESTS PUBLIC VPI_LIB)