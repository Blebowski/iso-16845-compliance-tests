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
    PLI_LIB OBJECT

    #ghdl_pli_types.h
    #ghdl_vpi_user.h

    #pli_utils.h
    #pli_handle_manager.h
    #SimulatorChannel.hpp
    #vpiComplianceLib.hpp

    pli_handle_manager.c
    pli_utils.c
    SimulatorChannel.cpp
    PliComplianceLib.cpp
)

add_compile_options(PLI_LIB -shared)
target_compile_definitions(PLI_LIB PUBLIC -DCTU_VIP_HIERARCHICAL_PATH=\"${CTU_VIP_HIERARCHICAL_PATH}\")

