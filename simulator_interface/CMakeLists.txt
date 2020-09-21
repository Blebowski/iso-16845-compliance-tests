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

###############################################################################
# I did not find a way how to link several shared libraries to GHDL VPI, so
# I build everything including sources from other libraries (model, testlib,
# etc...) to single shared library and link that one to GHDL!
###############################################################################
get_target_property(TEST_LIB_SOURCES_RAW TEST_LIB SOURCES)
get_target_property(CAN_LIB_SOURCES_RAW CAN_LIB SOURCES)
get_target_property(COMPLIANCE_TESTS_RAW COMPLIANCE_TESTS SOURCES)
get_target_property(VPI_LIB_RAW VPI_LIB SOURCES)

foreach (LIST_ITEM ${TEST_LIB_SOURCES_RAW})
    list(APPEND TEST_LIB_SOURCES "../test_lib/${LIST_ITEM}")
endforeach(LIST_ITEM)

foreach (LIST_ITEM ${CAN_LIB_SOURCES_RAW})
    list(APPEND CAN_LIB_SOURCES "../can_lib/${LIST_ITEM}")
endforeach(LIST_ITEM)

foreach (LIST_ITEM ${COMPLIANCE_TESTS_RAW})
    list(APPEND COMPLIANCE_LIB_SOURCES "../compliance_tests/${LIST_ITEM}")
endforeach(LIST_ITEM)

foreach (LIST_ITEM ${VPI_LIB_RAW})
    list(APPEND VPI_LIB_SOURCES "../vpi_lib/${LIST_ITEM}")
endforeach(LIST_ITEM)

add_library(
    SIMULATOR_INTERFACE_LIB SHARED

    simulator_interface.c

    ${VPI_LIB_SOURCES}
    ${CAN_LIB_SOURCES}
    ${TEST_LIB_SOURCES}
    ${COMPLIANCE_LIB_SOURCES}
)

target_compile_definitions(SIMULATOR_INTERFACE_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)

target_link_options(SIMULATOR_INTERFACE_LIB PUBLIC -pthread)
