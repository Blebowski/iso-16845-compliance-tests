
add_library(
    VPI_LIBRARY SHARED
    
    vpi_user.h
    _pli_types.h
    ghdl.c
)

add_compile_options(VPI_LIBRARY -fPIC -Wall -g -I -c -shared)