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
    GHDL_VPI_COSIM_LIB SHARED

    simulator_interface.c
    pli_handle_manager.c
    pli_utils.c
    SimulatorChannel.cpp
    PliComplianceLib.cpp
)

add_library(
    VCS_VHPI_COSIM_LIB SHARED

    simulator_interface.c
    pli_handle_manager.c
    pli_utils.c
    SimulatorChannel.cpp
    PliComplianceLib.cpp
)

add_library(
    NVC_VHPI_COSIM_LIB SHARED

    simulator_interface.c
    pli_handle_manager.c
    pli_utils.c
    SimulatorChannel.cpp
    PliComplianceLib.cpp
)

target_compile_definitions(GHDL_VPI_COSIM_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)
target_compile_definitions(VCS_VHPI_COSIM_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)
target_compile_definitions(NVC_VHPI_COSIM_LIB PUBLIC -D__LITTLE_ENDIAN_BITFIELD)

# Distinguish PLI kind for different libraries
target_compile_definitions(GHDL_VPI_COSIM_LIB PUBLIC PLI_KIND=0)
target_compile_definitions(VCS_VHPI_COSIM_LIB PUBLIC PLI_KIND=1)
target_compile_definitions(NVC_VHPI_COSIM_LIB PUBLIC PLI_KIND=2)

# Distinguish CTU_CAN_FD_VIP path
SET (GHDL_VPI_CTU_VIP_HIERARCHICAL_PATH "tb_top_ctu_can_fd/ctu_can_fd_vip_inst")
SET (VCS_VHPI_CTU_VIP_HIERARCHICAL_PATH ":TB_TOP_CTU_CAN_FD:CTU_CAN_FD_VIP_INST")
SET (NVC_VHPI_CTU_VIP_HIERARCHICAL_PATH ":TB_TOP_CTU_CAN_FD:CTU_CAN_FD_VIP_INST")

message(STATUS "GHDL VPI: CTU CAN FD VIP simulation hierarchy is: ${GHDL_VPI_CTU_VIP_HIERARCHICAL_PATH}")
message(STATUS "VCS VHPI: CTU CAN FD VIP simulation hierarchy is: ${VCS_VHPI_CTU_VIP_HIERARCHICAL_PATH}")
message(STATUS "NVC VHPI: CTU CAN FD VIP simulation hierarchy is: ${NVC_VHPI_CTU_VIP_HIERARCHICAL_PATH}")

target_compile_definitions(GHDL_VPI_COSIM_LIB PUBLIC CTU_VIP_HIERARCHICAL_PATH=\"${GHDL_VPI_CTU_VIP_HIERARCHICAL_PATH}\")
target_compile_definitions(VCS_VHPI_COSIM_LIB PUBLIC CTU_VIP_HIERARCHICAL_PATH=\"${VCS_VHPI_CTU_VIP_HIERARCHICAL_PATH}\")
target_compile_definitions(NVC_VHPI_COSIM_LIB PUBLIC CTU_VIP_HIERARCHICAL_PATH=\"${NVC_VHPI_CTU_VIP_HIERARCHICAL_PATH}\")

target_link_libraries(GHDL_VPI_COSIM_LIB PUBLIC CAN_LIB)
target_link_libraries(GHDL_VPI_COSIM_LIB PUBLIC TEST_LIB)
target_link_libraries(GHDL_VPI_COSIM_LIB PUBLIC COMPLIANCE_TESTS)

target_link_libraries(VCS_VHPI_COSIM_LIB PUBLIC CAN_LIB)
target_link_libraries(VCS_VHPI_COSIM_LIB PUBLIC TEST_LIB)
target_link_libraries(VCS_VHPI_COSIM_LIB PUBLIC COMPLIANCE_TESTS)

target_link_libraries(NVC_VHPI_COSIM_LIB PUBLIC CAN_LIB)
target_link_libraries(NVC_VHPI_COSIM_LIB PUBLIC TEST_LIB)
target_link_libraries(NVC_VHPI_COSIM_LIB PUBLIC COMPLIANCE_TESTS)

target_link_options(GHDL_VPI_COSIM_LIB PUBLIC -pthread)
target_link_options(VCS_VHPI_COSIM_LIB PUBLIC -pthread)
target_link_options(NVC_VHPI_COSIM_LIB PUBLIC -pthread)