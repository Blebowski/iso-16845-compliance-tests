
add_library(
    VPI_LIB SHARED

    _pli_types.h
    vpi_user.h
    vpi_compliance_lib.h
    vpi_utils.h

    vpi_utils.c
    vpi_compliance_lib.c
)

add_compile_options(VPI_LIB -fPIC -Wall -g -I -c -shared)
