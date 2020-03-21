
add_library(
    VPI_LIB SHARED

    _pli_types.h
    vpi_user.h
    vpi_utils.h
    SimulatorChannel.hpp
    vpi_compliance_lib.hpp
    
    vpi_utils.c
    SimulatorChannel.cpp
    vpi_compliance_lib.cpp
)

add_compile_options(VPI_LIB -fPIC -Wall -O0 -g -I -c -shared)
