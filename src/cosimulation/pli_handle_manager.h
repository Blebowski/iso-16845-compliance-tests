#ifndef PLI_HANDLE_MANAGER_H
#define PLI_HANDLE_MANAGER_H
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
 * @date 09.10.2020
 *
 * PLI handle (pointers to signals) manager. Does following:
 *      - Manages VPI/VHPI handles, so that they are not queried over VPI/VHPI
 *        multiple times (avoids memory leaks inside GHDL!
 *      - Locates CTU CAN FD VIP in hierarchy of HDL simulation.
 *
 * Handle manager maintains cache of handles to signals which has been already
 * obtained. If handle to a signal with equal name is queried, cached value is
 * returned instead.
 *
 * This assumes that we do not query handles to signals with the same name at
 * different places in hierarchy. This is reasonable assumption as all the
 * VPI/VHPI communication signals are located in Test Controller agent of
 * CTU CAN FD VIP (in single hierarchy)
 *****************************************************************************/

#include "pli_utils.h"

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
    T_PLI_HANDLE handle;
    char *signal_name;
    size_t signal_size;
    struct hlist_node *next;
};

/**
 * @brief Returns handle to signal in CTU CAN FD VIP test controller agent.
 * @returns Pointer to handle entry in list of handles.
 */
struct hlist_node* hman_get_ctu_vip_net_handle(const char *signal_name);

/**
 * @brief Should be called at the end of simulation to perform cleanup
 *        (free cached handles)
 */
void hman_cleanup();

#endif