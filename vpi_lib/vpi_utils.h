/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 * 
 *****************************************************************************/

#include <string.h>
#include <pthread.h>

#include "_pli_types.h"
#include "vpi_user.h"

#ifndef VPI_UTILS
#define VPI_UTILS

#define VPI_TAG "\033[1;33mVPI: \033[0m"

// Testbench control interface
#define VPI_SIGNAL_CLOCK "vpi_clk"
#define VPI_SIGNAL_CONTROL_REQ "vpi_control_req"
#define VPI_SIGNAL_CONTROL_GNT "vpi_control_gnt"
#define VPI_SIGNAL_ALLOW_TIME_FLOW "vpi_allow_time_flow"
#define VPI_SIGNAL_TEST_END "vpi_test_end"
#define VPI_SIGNAL_TEST_RESULT "vpi_test_result"

#define VPI_SIGNAL_TEST_NAME_LENGHT "vpi_test_name_lenght"
#define VPI_SIGNAL_TEST_NAME_ARRAY "vpi_test_name_array"

// Communication interface
#define VPI_SIGNAL_REQ "vpi_req"
#define VPI_SIGNAL_ACK "vpi_ack"
#define VPI_SIGNAL_CMD "vpi_cmd"
#define VPI_SIGNAL_DEST "vpi_dest"
#define VPI_SIGNAL_DATA_IN "vpi_data_in"
#define VPI_SIGNAL_DATA_IN_2 "vpi_data_in_2"
#define VPI_SIGNAL_DATA_OUT "vpi_data_out"

#define VPI_STR_BUF_IN "vpi_str_buf_in"
#define VPI_STR_BUF_SIZE 64  // Each character is 8 bit vector

// Size of vpi_data_in and vpi_data out vectors
#define VPI_DBUF_SIZE 64

/**
 * @brief Obtain VPI handle to a net in Simulator.
 * 
 * @param moduleHandle Handle to a module/entity containing the signal.
 * @param netName Name of the net.
 * @returns Handle to the net.
 * 
 * @warning This function should be called only in simulator context as result
 *          of simulator callback.
 */
vpiHandle get_net_handle(vpiHandle moduleHandle, const char *netName);


/**
 * @brief Drive value to top level net in Simulator. Signal shall be logic or
 *        logic vector.
 * 
 * @param signalName Name of the signal/net to be driven.
 * @param value Value to be driven to the signal. String shall have format:
 *                 "10UZXLH" for all values of std_logic_vector.
 * @returns 0 if succesfull, -1 otherwise
 * 
 * @warning This function should be called only in simulator context as result
 *          of simulator callback.
 */
int vpi_drive_str_value(const char *signalName, char *value);


/**
 * @brief Read value from top level net in Simulator. Signal shall be logic or
 *        logic vector.
 * 
 * @param signalName Name of the signal/net to read value from.
 * @param value Value read from the signal.
 * @returns 0 if succesfull, -1 otherwise
 * 
 * @warning This function should be called only in simulator context as result
 *          of simulator callback.
 */
int vpi_read_str_value(const char *signalName, char *retValue);


#endif
