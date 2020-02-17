
add_library(
    TEST_LIB

    # CAN namespace
    test_lib.h
    
    # Headers
    DriverItem.h
    MonitorItem.h
    TestSequence.h

    # Implementation
    DriverItem.cpp
    MonitorItem.cpp
    TestSequence.cpp
)

add_compile_options(TEST_LIB -g)
