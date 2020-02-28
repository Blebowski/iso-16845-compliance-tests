
add_library(
    TEST_LIB SHARED

    # CAN namespace
    test_lib.h
    
    # Headers
    DriverItem.h
    MonitorItem.h
    TestSequence.h
    TestLoader.h
    TestBase.h
    
    # Implementation
    DriverItem.cpp
    MonitorItem.cpp
    TestSequence.cpp
    TestLoader.cpp
    TestBase.cpp
)

add_compile_options(TEST_LIB -g -fPIC)
