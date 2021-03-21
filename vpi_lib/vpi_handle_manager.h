/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 09.10.2020
 * 
 * VPI handle (pointers to signals) manager. Does following:
 *      - Manages VPI handles, so that they are not queried over VPI multiple
 *        times (avoids memory leaks inside GHDL!
 *      - Locates CTU CAN FD VIP in hierarchy of HDL simulation.
 *
 * Handle manager maintains cache of handles to signals which has been already
 * obtained. If handle to a signal with equal name is queried, cached value is
 * returned instead.
 * 
 * This assumes that we do not query handles to signals with the same name at
 * different places in hierarchy. This is reasonable assumption as all the
 * VPI communication signals are located in Test Controller agent of CTU CAN
 * FD VIP (in single hierarchy)
 *****************************************************************************/

#include "vpi_user.h"

/**
 * @brief Handle List entry.
 * 
 * By buffering already queried handles and their names, in list/cache, we can
 * avoid querying the handle from HDL simulator multiple times. Instead, only
 * first time the handle is queried from HDL sim. All next queries search the
 * list and return cached version.
 * 
 * This helps performance-wise, and also avoids memory leaks in GHDL.
 */
struct hlist_node {
    vpiHandle handle;
    char *signal_name;
    int signal_size;
    struct hlist_node *next;
};

/**
 * @brief Returns handle to signal in CTU CAN FD VIP test controller agent.
 * @returns Pointer to handle entry in list of handles.
 */
struct hlist_node* hman_get_ctu_vip_net_handle(const char *signal_name);