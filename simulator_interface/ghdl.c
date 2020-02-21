/*
 * TODO: License
 */

#include <stdio.h>
#include <string.h>

#include "vpi_user.h"

#define VPI_TAG "VPI: "

// Top level VPI signals in VHDL TB
#define VPI_CONTROL_REQ "vpi_control_req"
#define VPI_CONTROL_GNT "vpi_control_gnt"


// Global variables
/*
vpiHandle topIterator;
vpiHandle topModule;
vpiHandle topScope;
*/

/**
 * Register hook on signal which gives away control to SW part of TB!
 * TODO: Describe more!!
 */
void sw_control_req_callback(struct t_cb_data*cb)
{
    vpi_printf("%s Simulator requests passing control to SW!\n", VPI_TAG);

    // Issue grant for SW
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle topScope = vpi_handle(vpiScope, topModule);
    vpiHandle netIterator = vpi_iterate(vpiNet, topScope);
    vpiHandle signalHandle;
    const char *sigName;

    while ((signalHandle = (vpiHandle)vpi_scan(netIterator)) != NULL)
    {
        sigName = vpi_get_str(vpiName, signalHandle);

        if (strcmp(sigName, VPI_CONTROL_GNT) == 0)
            goto issue_ack;
    }
    vpi_printf("%s Can't find %s signal", VPI_TAG, VPI_CONTROL_GNT);
    fprintf(stderr, "%s Can't find %s signal", VPI_TAG, VPI_CONTROL_GNT);

issue_ack:;
    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_value.value.str = "1";
    vpi_put_value(signalHandle, &vpi_value, NULL, vpiNoDelay);
    vpi_printf("%s Control passed to SW\n", VPI_TAG);

    // TODO: Here call the main test function which forks SystemC test object!
}

/**
 * Register hook on signal which gives away control to SW part of TB!
 * TODO: Describe more!!
 */
void vpi_start_of_sim(){

    vpi_printf("%s Simulation start callback\n", VPI_TAG);

    // Register hook on signal which gives away control to SW part of 
    // testbench

    // Getting common handles
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle topScope = vpi_handle(vpiScope, topModule);

    vpiHandle netIterator = vpi_iterate(vpiNet, topScope);
    vpiHandle signalHandle;
    const char *modName;

    vpi_printf("%s Registering TB request for passing control to SW on: %s\n",
               VPI_TAG, VPI_CONTROL_REQ);
    while ((signalHandle = (vpiHandle)vpi_scan(netIterator)) != NULL)
    {
        modName = vpi_get_str(vpiName, signalHandle);

        if (strcmp(modName, VPI_CONTROL_REQ) == 0)
            goto control_req;
    }

    vpi_printf("%s Top level signal: %s not found\n", VPI_TAG, VPI_CONTROL_REQ);
    fprintf(stderr, "%s Top level signal: %s not found\n", VPI_TAG, VPI_CONTROL_REQ);
    return;

control_req:;

    // This function should be called only once, it should not matter we make
    // the callback declarations static here!
    static s_vpi_time timeStruct = {vpiSimTime};
    static s_vpi_value valueStruct = {vpiBinStrVal};
    static s_cb_data cbDataStruct;

    cbDataStruct.reason = cbValueChange;
    cbDataStruct.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(&sw_control_req_callback);
    cbDataStruct.time = &timeStruct;
    cbDataStruct.value = &valueStruct;
    cbDataStruct.obj = signalHandle;

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