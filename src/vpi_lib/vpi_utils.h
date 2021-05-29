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
#define VPI_SIGNAL_CLOCK "pli_clk"
#define VPI_SIGNAL_CONTROL_REQ "pli_control_req"
#define VPI_SIGNAL_CONTROL_GNT "pli_control_gnt"
#define VPI_SIGNAL_TEST_END "pli_test_end"
#define VPI_SIGNAL_TEST_RESULT "pli_test_result"

#define VPI_SIGNAL_TEST_NAME_LENGHT "pli_test_name_lenght"
#define VPI_SIGNAL_TEST_NAME_ARRAY "pli_test_name_array"

// Communication interface
#define VPI_SIGNAL_REQ "pli_req"
#define VPI_SIGNAL_ACK "pli_ack"
#define VPI_SIGNAL_CMD "pli_cmd"
#define VPI_SIGNAL_DEST "pli_dest"
#define VPI_SIGNAL_DATA_IN "pli_data_in"
#define VPI_SIGNAL_DATA_IN_2 "pli_data_in_2"
#define VPI_SIGNAL_DATA_OUT "pli_data_out"

#define VPI_STR_BUF_IN "pli_str_buf_in"
#define VPI_STR_BUF_SIZE 64  // Each character is 8 bit vector

// Size of vpi_data_in and vpi_data out vectors
#define VPI_DBUF_SIZE 64


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
int vpi_drive_str_value(const char *signal_name, const char *value);


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
int vpi_read_str_value(const char *signal_name, char *ret_value);


#endif
