/*
 * TODO: License
 */

#include <stdio.h>
#include <string.h>

#include "vpi_user.h"
#include "vpi_utils.h"

//#include "../test_lib/TestLoader.h"

/**
 * Function imported from C++
 */
int RunCppTest(char *testName);


/**
 * Register hook on signal which gives away control to SW part of TB!
 * TODO: Describe more!!
 */
void sw_control_req_callback(struct t_cb_data*cb)
{
    vpi_printf("%s Simulator requests passing control to SW!\n", VPI_TAG);
    vpiDriveStrValue(VPI_SIGNAL_CONTROL_GNT, "1");
    vpi_printf("%s Control passed to SW\n", VPI_TAG);

    /* Obtain test name. This is an ugly hack since GHDL VPI does not support
     * passing strings or custom arrays! Passed via std_logic_vector by 
     * converting each character to ASCII bit vector */
    char testNameBinary[1024];
    char testName[128];
    memset(testNameBinary, 0, sizeof(testNameBinary));
    memset(testName, 0, sizeof(testName));
    vpiReadStrValue(VPI_SIGNAL_TEST_NAME_ARRAY, &(testNameBinary[0]));
    for (int i = 0; i < strlen(testNameBinary); i += 8)
    {
        char letter = 0;
        for (int j = 0; j < 8; j++)
            if (testNameBinary[i + j] == '1')
                letter |= 0x1 << (7 - j);
        testName[i / 8] = letter;
    }
    vpi_printf("%s Test name fetched from TB: \033[1;32m%s\n\033[0m", VPI_TAG, testName);

    RunCppTest(testName);
}

/**
 * Register hook on signal which gives away control to SW part of TB!
 * TODO: Describe more!!
 */
void vpi_start_of_sim(){

    vpi_printf("%s Simulation start callback\n", VPI_TAG);

    // Get request signal handle
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle reqHandle = getNetHandle(topModule, VPI_SIGNAL_CONTROL_REQ);
    if (reqHandle == NULL)
    {
        vpi_printf("%s Can't register request handle\n", VPI_TAG);
        fprintf(stderr, "%s Can't register request handle\n", VPI_TAG);
        return;
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
        return;
    }
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