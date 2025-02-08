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
 * @brief PLI interface for GHDL(VPI)/VCS(VHPI).
 *
 * @details Simulator <-> Library communication is done like so:
 *      - simulator calls "handle_register" because it detects linked library.
 *      - "handle_register" registers "pli_start_of_sim", which is called by
 *         simulator at simulation start (after all analysis and elaboration).
 *      - Simulation starts and GHDL calls "pli_start_of_sim" at time 0.
 *        This function registers:
 *          - PLI clock callback for synchronous communication between simulator
 *            and compliance library contexts (register_pli_clk_cb)
 *          - Callback for transfering control over TB to compliance library
 *            (register_control_transfer_cb, signal pli_control_req)
 *      - Simulation starts running and HDL side sets "pli_control_req", causing
 *         "sw_control_req_callback" to be called. This callback obtains test name
 *        to be by compliance test library (set by HDL on "pli_test_name" signal).
 *        It calls "RunCppTest" function which forks of test-thread and returns,
 *        letting simulator proceed further with simulation.
 *
 *      Since this moment on, two contexts live:
 *          - Simulator context (in which simulator runs)
 *          - Test context (in which compliance test lib runs)
 *
 *      These two communicate over shared memory interface. Test context controls
 *      the simulation (Agents and DUT) and when it is done running the test, it
 *      signals this back to simulator context ("pli_test_end" signal).
 *      Simulator then ends the simulation.
 *
 *      Each request from Test context to simulator is put to shared memory
 *      interface and it is picked up by Simulator context due to callbacks on
 *      "pli_clk" signal. Passing requests guarantees data consistency by
 *      using memory barriers (SW side) and hand-shake like operation (TB side)
 *      of this protocol.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <pli_lib.h>


/* Test information shared with test thread */
char test_name[128];

/**
 * Functions imported from C++
 */
void RunCppTest(char* test_name);
void ProcessPliClkCallback();

/**
 * Register hook on signal which gives away control to SW part of TB!
 */
void sw_control_req_callback(PLI_CB_ARG)
{
    UNUSED_PLI_CB_ARG

    char req_val;
    pli_read_str_value(PLI_SIGNAL_CONTROL_REQ, &(req_val));
    if (req_val != '1') {
        pli_printf(PLI_INFO, "Simulator control request dropped to zero");
        return;
    }

    pli_printf(PLI_INFO, "Simulator requests passing control to SW!");
    pli_drive_str_value(PLI_SIGNAL_CONTROL_GNT, "1");
    pli_printf(PLI_INFO, "Control passed to SW");

    char test_name_binary[1024];
    memset(test_name_binary, 0, sizeof(test_name_binary));
    memset(test_name, 0, sizeof(test_name));

    pli_read_str_value(PLI_SIGNAL_TEST_NAME_ARRAY, &(test_name_binary[0]));

    /*
     * GHDL VPI does not support passing strings or custom arrays.
     * Passed via std_logic_vector by converting each character to
     * ASCII bit vector.
     */
    for (size_t i = 0; i < strlen(test_name_binary); i += 8) {
        char letter = 0;
        for (size_t j = 0; j < 8; j++)
            if (test_name_binary[i + j] == '1')
                letter |= (char)((0x1) << (7 - j));
        test_name[i / 8] = letter;
    }

    pli_printf(PLI_INFO, "Test name fetched from TB: \033[1;31m%s\033[0m", test_name);
    RunCppTest(test_name);
}


/**
 * PLI clock callback. Called regularly from TB upon PLI clock which is generated
 * in simulation. Processes request from Test thread. Called in simulator context.
 */
void pli_clk_callback(PLI_CB_ARG)
{
    UNUSED_PLI_CB_ARG
    ProcessPliClkCallback();
}


/**
 * Registers callback for control transfer to SW test.
 */
int register_control_transfer_cb()
{
    pli_printf(PLI_INFO, "Registering callback for control request...");
    struct hlist_node* node = hman_get_ctu_vip_net_handle(PLI_SIGNAL_CONTROL_REQ);

    if (node == NULL)
    {
        pli_printf(PLI_INFO, "Can't get handle for %s", PLI_SIGNAL_CONTROL_REQ);
        return -1;
    }

    if (pli_register_cb(P_PLI_CB_VALUE_CHANGE, node->handle, &sw_control_req_callback) == NULL)
    {
        pli_printf(PLI_INFO, "Cannot register cbValueChange call back for %s", PLI_SIGNAL_CONTROL_REQ);
        return -2;
    }

    return 0;
}


/**
 * Registers PLI clock callback.
 */
int register_pli_clk_cb()
{
    struct hlist_node *node = hman_get_ctu_vip_net_handle(PLI_SIGNAL_CLOCK);

    if (node == NULL)
    {
        pli_printf(PLI_INFO, "Can't obtain request handle for %s", PLI_SIGNAL_CLOCK);
        return -1;
    }

    if (pli_register_cb(P_PLI_CB_VALUE_CHANGE, node->handle, &pli_clk_callback) == NULL)
    {
        pli_printf(PLI_INFO, "Cannot register cbValueChange call back for %s", PLI_SIGNAL_CLOCK);
        return -2;
    }

    return 0;
}


/**
 * Callback upon start of simulation.
 */
void pli_start_of_sim(PLI_CB_ARG)
{
    UNUSED_PLI_CB_ARG
    pli_printf(PLI_INFO, "Simulation start callback");

    // If order of registration is swapped, then the PLI_CLK callback stops
    // working in NVC once the control transfer callback is called!

    pli_printf(PLI_INFO, "Registering PLI clock callback");
    register_pli_clk_cb();
    pli_printf(PLI_INFO, "Done");

    pli_printf(PLI_INFO, "Registering callback for control to SW");
    register_control_transfer_cb();
    pli_printf(PLI_INFO, "Done");

    return;
}

/**
 * Callback upon end of simulation
 */
void pli_end_of_sim(PLI_CB_ARG)
{
    UNUSED_PLI_CB_ARG
    pli_printf(PLI_INFO, "End of simulation callback SW");
    hman_cleanup();
}


/**
 * Called by simulator upon entrance to simulation (registers all handles)
 */
void handle_register()
{
    /* Start of simulation hook */
    pli_printf(PLI_INFO, "Registering start of simulation callback...");
    if (pli_register_cb(P_PLI_CB_START_OF_SIMULATION, NULL, &pli_start_of_sim) == NULL) {
        pli_printf(PLI_ERROR, "Cannot register start of simulation callback callback");
        return;
    }
    pli_printf(PLI_INFO, "Done");

    /* End of simulation hook */
    pli_printf(PLI_INFO, "Registering end of simulation callback...");
    if (pli_register_cb(P_PLI_CB_END_OF_SIMULATION, NULL, &pli_end_of_sim) == NULL) {
        pli_printf (PLI_ERROR, "Cannot register end of simulation callback");
        return;
    }
    pli_printf(PLI_INFO, "Done");
}


/**
 * Start-up routines that simulator executes
 */

#if PLI_KIND == PLI_KIND_GHDL_VPI

void (*vlog_startup_routines[]) () =
{
  handle_register,
  0
};

#elif (PLI_KIND == PLI_KIND_VCS_VHPI) || (PLI_KIND == PLI_KIND_NVC_VHPI)

void (*vhpi_startup_routines[])() = {
   handle_register,
   0
};

#endif