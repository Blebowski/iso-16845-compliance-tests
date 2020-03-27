###############################################################################
# 
# Copyright (C) Ondrej Ille - All Rights Reserved
#  
# Copying, publishing, distributing of this file is stricly prohibited unless
# previously aggreed with author of this text.
# 
# Author: Ondrej Ille, <ondrej.ille@gmail.com>
# Date: 27.3.2020
# 
###############################################################################

cmake_minimum_required(VERSION 3.5)

project(CAN_FD_MODEL)

# Libraries
add_subdirectory(can_lib)
add_subdirectory(test_lib)
add_subdirectory(vpi_lib)
add_subdirectory(compliance_tests)

add_subdirectory(simulator_interface)

# Example executable
#add_executable(example apps/main.cpp)
add_executable(test apps/test.cpp)

#target_compile_options(example PUBLIC -Wall -g)
#target_link_libraries(example PUBLIC CAN_LIB)
#target_link_libraries(example PUBLIC TEST_LIB)
#target_link_libraries(example PUBLIC VPI_LIB)
#target_link_libraries(example PUBLIC COMPLIANCE_TESTS)

#target_link_options(example PUBLIC -pthread)

set (EXECUTABLE_OUTPUT_PATH bin)