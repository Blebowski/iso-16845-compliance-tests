
add_library(
    COMPLIANCE_TESTS SHARED

    # Headers
    ../compliance_tests_includes/TestDemo.h

    # Implementation
    TestDemo.cpp
)

add_compile_options(COMPLIANCE_TESTS -g -fPIC)

target_link_libraries(COMPLIANCE_TESTS PUBLIC VPI_LIB)