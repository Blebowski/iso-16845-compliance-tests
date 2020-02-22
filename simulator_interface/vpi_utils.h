/*
 *  TODO: License
 */
#include <string.h>

#include "_pli_types.h"
#include "vpi_user.h"

#ifndef VPI_UTILS
#define VPI_UTILS

#define VPI_TAG "VPI: "

// Testbench control interface
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
#define VPI_SIGNAL_DATA_OUT "vpi_data_out"

/**
 * 
 */
vpiHandle getNetHandle(vpiHandle moduleHandle, const char *netName);


/**
 *
 */
int vpiDriveStrValue(const char *signalName, char *value);


/**
 *
 */
int vpiDriveIntValue(const char *signalName, int value);


/**
 *
 */
int vpiReadIntValue(const char *signalName, int *retValue);


/**
 *
 */
int vpiReadStrValue(const char *signalName, char *retValue);


/**
 *
 */
int vpiWaitTillValue(const char *signalName, char *value);


/**
 *
 */
void vpi_full_handshake();


/**
 *
 */
void vpi_begin_handshake();


/**
 *
 */
void vpi_end_handshake();


#endif
