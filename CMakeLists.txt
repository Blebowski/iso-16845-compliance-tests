cmake_minimum_required(VERSION 3.5)

project(CAN_FD_MODEL)

# Libraries
add_subdirectory(can_lib)
add_subdirectory(test_lib)

# Example executable
add_executable(example apps/main.cpp)
target_compile_options(example PUBLIC -Wall -g)
target_link_libraries(example PUBLIC CAN_LIB)
target_link_libraries(example PUBLIC TEST_LIB)

set (EXECUTABLE_OUTPUT_PATH build)