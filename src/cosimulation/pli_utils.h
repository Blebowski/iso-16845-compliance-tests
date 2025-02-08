#ifndef PLI_UTILS_H
#define PLI_UTILS_H
/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 *
 * TODO: Comment
 *
 *****************************************************************************/

#include <string.h>
#include <pthread.h>


///////////////////////////////////////////////////////////////////////////////
// Common defines
///////////////////////////////////////////////////////////////////////////////

// Testbench control interface
#define PLI_SIGNAL_CLOCK "pli_clk"
#define PLI_SIGNAL_CONTROL_REQ "pli_control_req"
#define PLI_SIGNAL_CONTROL_GNT "pli_control_gnt"
#define PLI_SIGNAL_TEST_END "pli_test_end"
#define PLI_SIGNAL_TEST_RESULT "pli_test_result"

#define PLI_SIGNAL_TEST_NAME_LENGHT "pli_test_name_lenght"
#define PLI_SIGNAL_TEST_NAME_ARRAY "pli_test_name_array"

// Communication interface
#define PLI_SIGNAL_REQ "pli_req"
#define PLI_SIGNAL_ACK "pli_ack"
#define PLI_SIGNAL_CMD "pli_cmd"
#define PLI_SIGNAL_DEST "pli_dest"
#define PLI_SIGNAL_DATA_IN "pli_data_in"
#define PLI_SIGNAL_DATA_IN_2 "pli_data_in_2"
#define PLI_SIGNAL_DATA_OUT "pli_data_out"
#define PLI_SIGNAL_STR_BUF_IN "pli_str_buf_in"

#define PLI_REQ_SIZE 1
#define PLI_ACK_SIZE 1
#define PLI_CMD_SIZE 8
#define PLI_DEST_SIZE 8
#define PLI_DATA_IN_SIZE 64
#define PLI_DATA_IN_2_SIZE 64
#define PLI_DATA_OUT_SIZE 64
#define PLI_STR_BUF_IN_SIZE 512

// Each character is 8 bit vector
#define PLI_STR_BUF_MAX_MSG_LEN (PLI_STR_BUF_IN_SIZE/8)


#define PLI_KIND_GHDL_VPI 0
#define PLI_KIND_VCS_VHPI 1
#define PLI_KIND_NVC_VHPI 2


///////////////////////////////////////////////////////////////////////////////
// Switch between VPI and VHPI
///////////////////////////////////////////////////////////////////////////////

#if PLI_KIND == PLI_KIND_GHDL_VPI

#include "ghdl_pli_types.h"
#include "ghdl_vpi_user.h"

#define PLI_TAG "\033[1;33mVPI: \033[0m"
#define PLI_HIER_SEP "/"

// Types
#define T_PLI_HANDLE vpiHandle
#define T_PLI_REASON PLI_INT32
#define T_PLI_CB_ARGS void
#define PLI_CB_ARG T_PLI_CB_ARGS
#define UNUSED_PLI_CB_ARG

// Properties
#define P_PLI_NAME vpiName
#define P_PLI_FULL_NAME vpiFullName
#define P_PLI_MODULE vpiModule
#define P_PLI_SIZE vpiSize
#define P_PLI_SCOPE vpiScope
#define P_PLI_NET vpiNet
#define P_PLI_CB_VALUE_CHANGE cbValueChange
#define P_PLI_CB_START_OF_SIMULATION cbStartOfSimulation
#define P_PLI_CB_END_OF_SIMULATION cbEndOfSimulation

// Functions
#define PLI_GET_STR vpi_get_str
#define PLI_ITERATE vpi_iterate
#define PLI_SCAN vpi_scan
#define PLI_HANDLE vpi_handle
#define PLI_FREE vpi_free_object
#define PLI_GET vpi_get


#elif PLI_KIND == PLI_KIND_VCS_VHPI

// For VCS header file
//#define STD_VCS_VHPI

#include "vcs_vhpi_user.h"

#define PLI_TAG "\033[1;33mVHPI: \033[0m"
#define PLI_HIER_SEP ":"

// Types
#define T_PLI_HANDLE vhpiHandleT
#define T_PLI_REASON vhpiCbReasonT
#define T_PLI_CB_ARGS struct vhpiCbDataS*
#define PLI_CB_ARG T_PLI_CB_ARGS data
#define UNUSED_PLI_CB_ARG (void)(data);

// Properties
#define P_PLI_NAME vhpiNameP
#define P_PLI_FULL_NAME vhpiFullNameP
#define P_PLI_MODULE vhpiCompInstStmts
#define P_PLI_SIZE vhpiSizeP

#define P_PLI_SCOPE vhpiUpperRegion
#define P_PLI_NET vhpiSigDecls
#define P_PLI_CB_VALUE_CHANGE vhpiCbValueChange
#define P_PLI_CB_START_OF_SIMULATION vhpiCbStartOfSimulation
#define P_PLI_CB_END_OF_SIMULATION vhpiCbEndOfSimulation

// Functions
#define PLI_GET_STR vhpi_get_str
#define PLI_ITERATE vhpi_iterator
#define PLI_SCAN vhpi_scan
#define PLI_HANDLE vhpi_handle
#define PLI_FREE vhpi_release_handle
#define PLI_GET vhpi_get

#elif PLI_KIND == PLI_KIND_NVC_VHPI

// For VCS header file
//#define STD_VCS_VHPI

#include "nvc_vhpi_user.h"

#define PLI_TAG "\033[1;33mVHPI: \033[0m"
#define PLI_HIER_SEP ":"

// Types
#define T_PLI_HANDLE vhpiHandleT
#define T_PLI_REASON int32_t
#define T_PLI_CB_ARGS const struct vhpiCbDataS*
#define PLI_CB_ARG T_PLI_CB_ARGS data
#define UNUSED_PLI_CB_ARG (void)(data);

// Properties
#define P_PLI_NAME vhpiNameP
#define P_PLI_FULL_NAME vhpiFullNameP
#define P_PLI_MODULE vhpiCompInstStmts
#define P_PLI_SIZE vhpiSizeP

#define P_PLI_SCOPE vhpiUpperRegion
#define P_PLI_NET vhpiSigDecls
#define P_PLI_CB_VALUE_CHANGE vhpiCbValueChange
#define P_PLI_CB_START_OF_SIMULATION vhpiCbStartOfSimulation
#define P_PLI_CB_END_OF_SIMULATION vhpiCbEndOfSimulation

// Functions
#define PLI_GET_STR vhpi_get_str
#define PLI_ITERATE vhpi_iterator
#define PLI_SCAN vhpi_scan
#define PLI_HANDLE vhpi_handle
#define PLI_FREE vhpi_release_handle
#define PLI_GET vhpi_get

// Unknown
#else
    #error Invalid PLI_KIND. Set to either 0 (VPI), 1 (VCS VHPI) or 2 (NVC VHPI)
#endif

typedef enum {
    PLI_DEBUG,
    PLI_INFO,
    PLI_ERROR
} t_pli_msg_severity;

/**
 * @brief Drive value to net in Simulator. Signal shall be logic or logic vector.
 *
 * @param signalName Name of the signal/net to be driven.
 * @param value Value to be driven to the signal. String shall have format:
 *                 "10UZXLH" for all values of std_logic_vector.
 * @returns 0 if succesfull, -1 otherwise
 *
 * @warning This function should be called only in simulator context as result
 *          of simulator callback.
 */
int pli_drive_str_value(const char *signal_name, const char *value);


/**
 * @brief Read value from net in Simulator. Signal shall be logic or logic vector.
 *
 * @param signalName Name of the signal/net to read value from.
 * @param value Value read from the signal.
 * @returns 0 if succesfull, -1 otherwise
 *
 * @warning This function should be called only in simulator context as result
 *          of simulator callback.
 */
int pli_read_str_value(const char *signal_name, char *ret_value);


/**
 * @brief Register callback
 * @param reason to call the callback
 * @param handle Simulation handle to object on which callback shall be registered
 * @param
 */
T_PLI_HANDLE pli_register_cb(T_PLI_REASON reason, T_PLI_HANDLE handle, void (*cb_fnc)(T_PLI_CB_ARGS));


/**
 * @brief Universal PLI print
 * @param severity Severity of the message, see t_pli_msg_severity
 * @param fmt Message format to be printed in simulator log
   @param ... Parameters for message format
 * @param
 */
void pli_printf(t_pli_msg_severity severity, const char *fmt, ...);


void* pli_malloc(size_t size);

#endif
