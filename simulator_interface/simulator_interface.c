/*
 * TODO: License
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../vpi_lib/vpi_user.h"
#include "../vpi_lib/vpi_utils.h"

// Test information shared with test thread
char testName[128];

/**
 * Function imported from C++
 */
void* runCppTest(char* testName);

/**
 * Register hook on signal which gives away control to SW part of TB!
 * TODO: Describe more!!
 */
void sw_control_req_callback(struct t_cb_data*cb)
{
    vpi_printf("%s Simulator requests passing control to SW!\n", VPI_TAG);
    vpi_drive_str_value(VPI_SIGNAL_CONTROL_GNT, "1");
    vpi_printf("%s Control passed to SW\n", VPI_TAG);

    /* Obtain test name. This is an ugly hack since GHDL VPI does not support
     * passing strings or custom arrays! Passed via std_logic_vector by 
     * converting each character to ASCII bit vector */
    char testNameBinary[1024];
    memset(testNameBinary, 0, sizeof(testNameBinary));
    memset(testName, 0, sizeof(testName));
    vpi_read_str_value(VPI_SIGNAL_TEST_NAME_ARRAY, &(testNameBinary[0]));
    for (int i = 0; i < strlen(testNameBinary); i += 8)
    {
        char letter = 0;
        for (int j = 0; j < 8; j++)
            if (testNameBinary[i + j] == '1')
                letter |= 0x1 << (7 - j);
        testName[i / 8] = letter;
    }
    vpi_printf("%s Test name fetched from TB: \033[1;31m%s\n\033[0m", VPI_TAG, testName);

    runCppTest(testName);
}

/**
 * 
 */
int register_start_of_sim_cb()
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle reqHandle = get_net_handle(topModule, VPI_SIGNAL_CONTROL_REQ);
    if (reqHandle == NULL)
    {
        vpi_printf("%s Can't register request handle\n", VPI_TAG);
        fprintf(stderr, "%s Can't register request handle\n", VPI_TAG);
        return -1;
    }

    // This function should be called only once, it should not matter we make
    // the callback declarations static here!
    static s_vpi_time timeStruct = {vpiSimTime};
    static s_vpi_value valueStruct = {vpiBinStrVal};
    static s_cb_data cbDataStruct;

    // Register hook for function which gives away control of TB to SW!
    cbDataStruct.reason = cbValueChange;
    cbDataStruct.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(&sw_control_req_callback);
    cbDataStruct.time = &timeStruct;
    cbDataStruct.value = &valueStruct;
    cbDataStruct.obj = reqHandle;

    if (vpi_register_cb (&cbDataStruct) == NULL)
    {
        vpi_printf ("%s Cannot register cbValueChange call back\n", VPI_TAG);
        fprintf(stderr, "%s Cannot register cbValueChange call back\n", VPI_TAG);
        return -2;
    }
    return 0;
}


int register_mutex_lock_callback()
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle reqHandle = get_net_handle(topModule, VPI_MUTEX_LOCK);
    if (reqHandle == NULL)
    {
        vpi_printf("%s Can't register request handle\n", VPI_TAG);
        fprintf(stderr, "%s Can't register request handle\n", VPI_TAG);
        return -1;
    }

    // This function should be called only once, it should not matter we make
    // the callback declarations static here!
    static s_vpi_time timeStruct = {vpiSimTime};
    static s_vpi_value valueStruct = {vpiBinStrVal};
    static s_cb_data cbDataStruct;

    // Register hook for function which gives away control of TB to SW!
    cbDataStruct.reason = cbValueChange;
    cbDataStruct.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(&lock_handshake_mutex);
    cbDataStruct.time = &timeStruct;
    cbDataStruct.value = &valueStruct;
    cbDataStruct.obj = reqHandle;

    if (vpi_register_cb (&cbDataStruct) == NULL)
    {
        vpi_printf ("%s Cannot register cbValueChange call back\n", VPI_TAG);
        fprintf(stderr, "%s Cannot register cbValueChange call back\n", VPI_TAG);
        return -2;
    }
    return 0;
}


int register_mutex_unlock_callback()
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle reqHandle = get_net_handle(topModule, VPI_MUTEX_UNLOCK);
    if (reqHandle == NULL)
    {
        vpi_printf("%s Can't register request handle\n", VPI_TAG);
        fprintf(stderr, "%s Can't register request handle\n", VPI_TAG);
        return -1;
    }

    // This function should be called only once, it should not matter we make
    // the callback declarations static here!
    static s_vpi_time timeStruct = {vpiSimTime};
    static s_vpi_value valueStruct = {vpiBinStrVal};
    static s_cb_data cbDataStruct;

    // Register hook for function which gives away control of TB to SW!
    cbDataStruct.reason = cbValueChange;
    cbDataStruct.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(&unlock_handshake_mutex);
    cbDataStruct.time = &timeStruct;
    cbDataStruct.value = &valueStruct;
    cbDataStruct.obj = reqHandle;

    if (vpi_register_cb (&cbDataStruct) == NULL)
    {
        vpi_printf ("%s Cannot register cbValueChange call back\n", VPI_TAG);
        fprintf(stderr, "%s Cannot register cbValueChange call back\n", VPI_TAG);
        return -2;
    }
    return 0;
}

/**
 * Register hook on signal which gives away control to SW part of TB!
 * TODO: Describe more!!
 */
void vpi_start_of_sim(){

    vpi_printf("%s Simulation start callback\n", VPI_TAG);

    // Get request signal handle
    vpi_printf("%s Registring callback for control to SW\n", VPI_TAG);
    if (register_start_of_sim_cb() != 0)
        return;
    vpi_printf("%s Done\n", VPI_TAG);

    // Initialize mutex for handshake communication between simulator and test
    vpi_printf("%s Initializing simulator <-> test handshake mutex \n", VPI_TAG);
    if (pthread_mutex_init(&handshakeMutex, NULL) != 0)
    {
        vpi_printf("%s Cannot initialize handshake mutex for simulator <-> test communication\n", VPI_TAG);
        fprintf(stderr, "%s Cannot initialize handshake mutex for simulator <-> test communication\n", VPI_TAG);
        return;
    }

    // Register callbacks for lock/unlock mutex for handshake interface
    vpi_printf("%s Registring callback for handshake mutex lock SW\n", VPI_TAG);
    if (register_mutex_lock_callback() != 0)
        return;
    vpi_printf("%s Done\n", VPI_TAG);

    vpi_printf("%s Registring callback for handshake mutex unlock SW\n", VPI_TAG);
    if (register_mutex_unlock_callback() != 0)
        return;
    vpi_printf("%s Done\n", VPI_TAG);

    return;
}


/**
 * Initial callback registration
 * TODO: Describe more!!
 */
void handleRegister()
{
    s_cb_data cb_start;
  
    // Start of simulation hook
    vpi_printf("%s Registering start of simulation callback...\n", VPI_TAG);
    cb_start.reason = cbStartOfSimulation;
    cb_start.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(&vpi_start_of_sim);
    cb_start.user_data = NULL;
    if (vpi_register_cb (&cb_start) == NULL)
    {
        vpi_printf ("%s Cannot register cbStartOfSimulation call back\n", VPI_TAG);
        fprintf(stderr, "%s Cannot register cbStartOfSimulation call back\n", VPI_TAG);
        return;
    }
    vpi_printf("%s Done\n", VPI_TAG);
}


/**
 * Defined by VPI standard where simulator will look for callbacks when it loads
 * VPI module!!
 */
void (*vlog_startup_routines[]) () =
{
  handleRegister,
  0
};