/*
 *  TODO: License
 */
#include <string.h>
#include <pthread.h>

#include "_pli_types.h"
#include "vpi_user.h"

#ifndef VPI_UTILS
#define VPI_UTILS

#define VPI_TAG "\033[1;33mVPI: \033[0m"

// Testbench control interface
#define VPI_SIGNAL_CONTROL_REQ "vpi_control_req"
#define VPI_SIGNAL_CONTROL_GNT "vpi_control_gnt"
#define VPI_SIGNAL_ALLOW_TIME_FLOW "vpi_allow_time_flow"
#define VPI_SIGNAL_TEST_END "vpi_test_end"
#define VPI_SIGNAL_TEST_RESULT "vpi_test_result"

#define VPI_SIGNAL_TEST_NAME_LENGHT "vpi_test_name_lenght"
#define VPI_SIGNAL_TEST_NAME_ARRAY "vpi_test_name_array"

// Mutex acquire interface
#define VPI_MUTEX_LOCK "vpi_mutex_lock"
#define VPI_MUTEX_UNLOCK "vpi_mutex_unlock"

// Communication interface
#define VPI_SIGNAL_REQ "vpi_req"
#define VPI_SIGNAL_ACK "vpi_ack"
#define VPI_SIGNAL_CMD "vpi_cmd"
#define VPI_SIGNAL_DEST "vpi_dest"
#define VPI_SIGNAL_DATA_IN "vpi_data_in"
#define VPI_SIGNAL_DATA_OUT "vpi_data_out"

// Size of vpi_data_in and vpi_data out vectors
#define VPI_DBUF_SIZE 64

// Handshake mutex
extern pthread_mutex_t handshakeMutex;

/**
 * 
 */
void lockHandshakeMutex();

/**
 * 
 */
void unlockHandshakeMutex();

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
int vpiReadStrValue(const char *signalName, char *retValue);


/**
 *
 */
int vpiWaitTillStrValue(const char *signalName, char *value);


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

/**
 * 
 */
void vpi_info(char *);


/**
 * 
 */
//void vpi_info(char *);

#endif
