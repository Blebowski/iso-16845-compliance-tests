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

add_library(
    VPI_LIB OBJECT

    _pli_types.h
    vpi_user.h
    vpi_utils.h
    vpi_handle_manager.h
    SimulatorChannel.hpp
    vpiComplianceLib.hpp
    
    vpi_handle_manager.c
    vpi_utils.c
    SimulatorChannel.cpp
    vpiComplianceLib.cpp
)

add_compile_options(VPI_LIB -fPIC -Wall -O0 -g -I -c -shared)
