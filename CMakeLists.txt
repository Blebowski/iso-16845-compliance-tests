###############################################################################
#
# ISO16845 Compliance tests Copyright (C) 2021-present Ondrej Ille
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this SW component and associated documentation files (the "Component"),
# to use, copy, modify, merge, publish, distribute the Component for
# educational, research, evaluation, self-interest purposes. Using the
# Component for commercial purposes is forbidden unless previously agreed with
# Copyright holder.
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Component.
#
# THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
# IN THE COMPONENT.
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
