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
    SIMULATOR_INTERFACE_LIB SHARED

    ghdl_vpi_interface.c

    $<TARGET_OBJECTS:VPI_LIB>
    $<TARGET_OBJECTS:CAN_LIB>
    $<TARGET_OBJECTS:TEST_LIB>
    $<TARGET_OBJECTS:COMPLIANCE_TESTS>
)

target_compile_definitions(SIMULATOR_INTERFACE_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)

target_link_options(SIMULATOR_INTERFACE_LIB PUBLIC -pthread)
