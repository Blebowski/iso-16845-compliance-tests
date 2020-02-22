
# I did not find a way how to link several shared libraries to GHDL VPI, so
# I build everything including sources from other libraries (model, testlib,
# etc...) to single shared library and link that one to GHDL!
get_target_property(TEST_LIB_SOURCES_RAW TEST_LIB SOURCES)
get_target_property(CAN_LIB_SOURCES_RAW CAN_LIB SOURCES)

foreach (LIST_ITEM ${TEST_LIB_SOURCES_RAW})
    list(APPEND TEST_LIB_SOURCES "../test_lib/${LIST_ITEM}")
endforeach(LIST_ITEM)

foreach (LIST_ITEM ${CAN_LIB_SOURCES_RAW})
    list(APPEND CAN_LIB_SOURCES "../can_lib/${LIST_ITEM}")
endforeach(LIST_ITEM)


add_library(
    VPI_LIBRARY SHARED
    
    vpi_user.h
    vpi_utils.h
    vpi_compliance_lib.h
    _pli_types.h

    vpi_utils.c
    vpi_compliance_lib.c
    ghdl.c

    ../test_lib/test_lib.h
    
    # Headers
    ../test_lib/DriverItem.h
    ../test_lib/MonitorItem.h
    ../test_lib/TestSequence.h
    ../test_lib/TestLoader.h

    # Implementation
    ../test_lib/DriverItem.cpp
    ../test_lib/MonitorItem.cpp
    ../test_lib/TestSequence.cpp
    ../test_lib/TestLoader.cpp

    ${CAN_LIB_SOURCES}
    ${TEST_LIB_SOURCES}
)

add_compile_options(VPI_LIBRARY -fPIC -Wall -g -I -c -shared)