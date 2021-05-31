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

project(ISO16845_COMPLIANCE_TESTS)

SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
# Configure hierarchical path of CTU CAN FD VIP instantiated in HDL simulation.
# Use '/' as hierarchy delimiter.
###############################################################################
SET (CTU_VIP_HIERARCHICAL_PATH "tb_top_ctu_can_fd/ctu_can_fd_vip_inst")

MESSAGE("Build type: " CMAKE_BUILD_TYPE)
add_compile_options(-fPIC -Wall -Wextra -Wpedantic -I -c)

IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_definitions("-DDEBUG_BUILD")
ELSEIF (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
ENDIF()

FIND_PROGRAM(CPP_CHECK "cppcheck")
IF (CPP_CHECK)
    set(CMAKE_CXX_CPPCHECK "cppcheck")
ENDIF()

enable_testing()
SET(CTEST_OUTPUT_ON_FAILURE TRUE)

include_directories(src)

add_subdirectory(src)
add_subdirectory(test)

set (EXECUTABLE_OUTPUT_PATH bin)
