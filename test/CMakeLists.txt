macro(add_can_lib_test TEST_FILE TEST_NAME)
    add_executable(${TEST_NAME}_BIN ${TEST_FILE})    
    target_link_libraries(${TEST_NAME}_BIN $<TARGET_OBJECTS:CAN_LIB>)
    add_test(${TEST_NAME} ${TEST_NAME}_BIN)
endmacro()


add_can_lib_test(ExampleTest.cpp EXAMPLE_TEST)
add_can_lib_test(FrameFlagsTest.cpp FRAME_FLAGS_TEST)
add_can_lib_test(FrameTest.cpp FRAME_TEST)

