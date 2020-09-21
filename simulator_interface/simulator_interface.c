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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../vpi_lib/vpi_user.h"
#include "../vpi_lib/vpi_utils.h"

/* Test information shared with test thread */
char test_name[128];

/**
 * Function imported from C++
 */
void RunCppTest(char* test_name);
void ProcessVpiClkCallback();


/**
 * VPI Clock callback
 */
s_vpi_time vpi_clk_time_data = {
    .type = vpiSimTime,
    .high = 0,
    .low = 0,
    .real = 0.0
};

s_vpi_value vpi_clk_value_data = {
    .format = vpiBinStrVal
};

s_cb_data vpi_clk_cb_struct;


/**
 * Register hook on signal which gives away control to SW part of TB!
 */
void sw_control_req_callback()
{
    vpi_printf("%s Simulator requests passing control to SW!\n", VPI_TAG);
    vpi_drive_str_value(VPI_SIGNAL_CONTROL_GNT, "1");
    vpi_printf("%s Control passed to SW\n", VPI_TAG);

    /* Obtain test name. This is an ugly hack since GHDL VPI does not support
     * passing strings or custom arrays! Passed via std_logic_vector by 
     * converting each character to ASCII bit vector */
    char testNameBinary[1024];
    memset(testNameBinary, 0, sizeof(testNameBinary));
    memset(test_name, 0, sizeof(test_name));
    vpi_read_str_value(VPI_SIGNAL_TEST_NAME_ARRAY, &(testNameBinary[0]));
    for (size_t i = 0; i < strlen(testNameBinary); i += 8)
    {
        char letter = 0;
        for (int j = 0; j < 8; j++)
            if (testNameBinary[i + j] == '1')
                letter |= 0x1 << (7 - j);
        test_name[i / 8] = letter;
    }
    vpi_printf("%s Test name fetched from TB: \033[1;31m%s\n\033[0m", VPI_TAG, test_name);

    RunCppTest(test_name);
}


/**
 * VPI clock callback. Called regularly from TB upon VPI clock which is internally
 * generated. Processes request from Test thread. Called in simulator context.
 */
void vpi_clk_callback()
{
    ProcessVpiClkCallback();
}


/**
 * Registers callback for control transfer to SW test.
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

    /* 
     * This function should be called only once, it should not matter we make
     * the callback declarations static here!
     * TODO: Why is it static?
     */
    static s_vpi_time timeStruct = {
        .type = vpiSimTime,
        .high = 0,
        .low = 0,
        .real = 0.0
    };
    static s_vpi_value valueStruct = {
        .format = vpiBinStrVal
    };
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

/**
 * Registers VPI clock callback.
 */
int register_vpi_clk_cb()
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle reqHandle = get_net_handle(topModule, VPI_SIGNAL_CLOCK);
    if (reqHandle == NULL)
    {
        vpi_printf("%s Can't register request handle\n", VPI_TAG);
        fprintf(stderr, "%s Can't register request handle\n", VPI_TAG);
        vpi_free_object(reqHandle);
        vpi_free_object(topModule);
        vpi_free_object(topIterator);
        return -1;
    }

    vpi_clk_cb_struct.reason = cbValueChange;
    vpi_clk_cb_struct.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(&vpi_clk_callback);
    vpi_clk_cb_struct.time = &vpi_clk_time_data;
    vpi_clk_cb_struct.value = &vpi_clk_value_data;
    vpi_clk_cb_struct.obj = reqHandle;
 
    if (vpi_register_cb(&vpi_clk_cb_struct) == NULL)
    {
        vpi_printf("%s Cannot register VPI clock callback\n", VPI_TAG);
        fprintf(stderr, "%s Cannot register VPI clock callback\n", VPI_TAG);
        vpi_free_object(reqHandle);
        vpi_free_object(topModule);
        vpi_free_object(topIterator);
        return -2;
    }

    vpi_free_object(reqHandle);
    vpi_free_object(topModule);
    vpi_free_object(topIterator);

    return 0;
}


/**
 * Callback upon start of simulation.
 */
void vpi_start_of_sim(){

    vpi_printf("%s Simulation start callback\n", VPI_TAG);

    vpi_printf("%s Registring callback for control to SW\n", VPI_TAG);
    if (register_start_of_sim_cb() != 0)
        return;
    vpi_printf("%s Done\n", VPI_TAG);

    vpi_printf("%s Registering VPI clock callback\n", VPI_TAG);
    if (register_vpi_clk_cb() != 0)
        return;
    vpi_printf("%s Done\n", VPI_TAG);

    return;
}


/**
 * Called by simulator upon entrance to simulation (registers all handles)
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