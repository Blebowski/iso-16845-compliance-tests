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
 *****************************************************************************/

#include <string.h>
#include <pthread.h>

#include "ghdl_pli_types.h"
#include "ghdl_vpi_user.h"


#define PLI_TAG "\033[1;33mVPI: \033[0m"

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

#define PLI_STR_BUF_IN "pli_str_buf_in"
#define PLI_STR_BUF_SIZE 64  // Each character is 8 bit vector

// Size of pli_data_in and pli_data out vectors
#define PLI_DBUF_SIZE 64


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


#endif
