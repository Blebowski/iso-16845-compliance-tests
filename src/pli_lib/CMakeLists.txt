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

    pli_handle_manager.c
    pli_utils.c
    SimulatorChannel.cpp
    PliComplianceLib.cpp
)

#add_compile_options(PLI_LIB -shared)


