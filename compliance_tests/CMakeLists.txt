
add_library(
    COMPLIANCE_TESTS SHARED

    TestDemo.cpp
)

add_compile_options(COMPLIANCE_TESTS -g -fPIC -Wall -O0)

target_link_libraries(COMPLIANCE_TESTS PUBLIC VPI_LIB)