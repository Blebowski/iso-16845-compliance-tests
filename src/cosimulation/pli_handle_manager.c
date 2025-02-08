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
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "pli_handle_manager.h"


/* Handle to CTU CAN FD VIP - Test controller agent module */
static T_PLI_HANDLE ctu_vip_handle = NULL;

/* List of handles which has been already queried by library */
static struct hlist_node *list_head = NULL;


/**
 * @brief Searches through module instances recursively if module matches
 *        the name.
 * @param module Handle to module to be searched
 * @param exp_name Expected name of the module
 */
static void hman_search_ctu_vip_handle(T_PLI_HANDLE module, char *exp_name)
{
    pli_printf(PLI_DEBUG, "hman_search_ctu_vip_handle: %s", exp_name);

#if PLI_KIND == PLI_KIND_GHDL_VPI
    char *curr_name = vpi_get_str(vpiName, module);
    pli_printf(PLI_DEBUG, "Checking path: %s", curr_name);

    if (!strcmp(exp_name, curr_name)) {
        vpiHandle mod_it = vpi_iterate(vpiModule, module);
        vpiHandle mod_tmp;

        /* We descend to VIP instance, we have no more paths */
        char *name = strtok(NULL, "/");
        if (name == NULL) {
            ctu_vip_handle = module;
            return;
        }

        while ((mod_tmp = (vpiHandle)vpi_scan(mod_it)) != NULL)
            hman_search_ctu_vip_handle(mod_tmp, name);
    }

#elif PLI_KIND == PLI_KIND_VCS_VHPI
    ctu_vip_handle = vhpi_handle_by_name(CTU_VIP_HIERARCHICAL_PATH, NULL);

#elif PLI_KIND == PLI_KIND_NVC_VHPI
    ctu_vip_handle = vhpi_handle_by_name(CTU_VIP_HIERARCHICAL_PATH, NULL);

#endif
}


/**
 * @returns Handle to CTU CAN FD VIP module inside HDL simulation.
 */
static T_PLI_HANDLE hman_get_ctu_vip_handle()
{
    pli_printf(PLI_DEBUG, "hman_get_ctu_vip_handle");

    /* Search upon first request and cache the handle */
    if (ctu_vip_handle == NULL)
    {
        pli_printf(PLI_DEBUG, "Searching for CTU CAN FD VIP module: %s",
                   CTU_VIP_HIERARCHICAL_PATH);

        /* Finds the handle and assigns to "ctu_vip_handle" */
        char *full_path;

#if PLI_KIND == PLI_KIND_GHDL_VPI
        char ctu_vip_path[] = CTU_VIP_HIERARCHICAL_PATH;
        char *top_name = strtok(ctu_vip_path, PLI_HIER_SEP);

        vpiHandle top_mod_it = vpi_iterate(vpiModule, NULL);
        vpiHandle top_mod_h = vpi_scan(top_mod_it);

        hman_search_ctu_vip_handle(top_mod_h, top_name);
        vpi_free_object(top_mod_it);

        full_path = vpi_get_str(vpiFullName, ctu_vip_handle);

#elif PLI_KIND == PLI_KIND_VCS_VHPI
        hman_search_ctu_vip_handle(NULL, 0);
        full_path = vhpi_get_str(vhpiFullNameP, ctu_vip_handle);

#elif PLI_KIND == PLI_KIND_NVC_VHPI
        hman_search_ctu_vip_handle(NULL, 0);
        full_path = vhpi_get_str(vhpiFullNameP, ctu_vip_handle);

#endif

        pli_printf(PLI_INFO, "Found CTU CAN FD VIP is: %s", full_path, ctu_vip_handle);
    }

    return ctu_vip_handle;
}


/**
 * @brief Creates handle to signal in CTU CAN FD VIP.
 * @param Signal_name name whose handle to create.
 * @returns PLI handle to signal.
 */
static T_PLI_HANDLE hman_create_ctu_vip_signal_handle(const char *signal_name)
{
    pli_printf(PLI_DEBUG, "hman_create_ctu_vip_signal_handle: %s", signal_name);

#if PLI_KIND == PLI_KIND_GHDL_VPI
    // Search through all signals in ctu_van_fd_vip handle
    vpiHandle ctu_scope_h = vpi_handle(vpiScope, hman_get_ctu_vip_handle());
    vpiHandle net_iterator = vpi_iterate(vpiNet, ctu_scope_h);
    vpiHandle signal_handle;
    const char *name;

    while ((signal_handle = (vpiHandle)vpi_scan(net_iterator)) != NULL)
    {
        name = vpi_get_str(vpiName, signal_handle);
        pli_printf(PLI_DEBUG, "Searching for signal: %s, Checking signal: %s",
                              signal_name, name);

        if (strcmp(name, signal_name) == 0)
        {
            pli_printf(PLI_DEBUG, "Found handle for: %s", signal_name);
            vpi_free_object(net_iterator);
            vpi_free_object(ctu_scope_h);
            return signal_handle;
        }
    }
    vpi_free_object(net_iterator);
    vpi_free_object(ctu_scope_h);

#elif PLI_KIND == PLI_KIND_VCS_VHPI
    // Get signals directly via VHPI
    char full_name[1024];
    memset(full_name, 0, sizeof(full_name));
    sprintf(full_name, "%s:%s", CTU_VIP_HIERARCHICAL_PATH, signal_name);

    // VCS VHDL signals are converted to upper-case
    char *curr = full_name;
    while (*curr) {
        *curr = (char)toupper((unsigned char) *curr);
        curr++;
    }

    vhpiHandleT sig_handle = vhpi_handle_by_name(full_name, NULL);
    if (sig_handle != NULL)
        return sig_handle;

#elif PLI_KIND == PLI_KIND_NVC_VHPI
    // Get signals directly via VHPI
    char full_name[1024];
    memset(full_name, 0, sizeof(full_name));
    sprintf(full_name, "%s:%s", CTU_VIP_HIERARCHICAL_PATH, signal_name);

    // VCS VHDL signals are converted to upper-case
    char *curr = full_name;
    while (*curr) {
        *curr = (char)toupper((unsigned char) *curr);
        curr++;
    }

    vhpiHandleT sig_handle = vhpi_handle_by_name(full_name, NULL);
    if (sig_handle != NULL)
        return sig_handle;

#endif

    pli_printf(PLI_ERROR, "Can't find handle for signal %s", signal_name);

    return NULL;
}


/**
 * @brief Searches handle list for handle with matching signal name.
 * @param signal_name Name of signal whose handle to search for
 * @param Pointer to handle node in Handle list.
 */
static struct hlist_node* hman_search_handle_list(const char *signal_name)
{
    pli_printf(PLI_DEBUG, "hman_search_handle_list: %s", signal_name);

    if (list_head == NULL)
        return NULL;

    struct hlist_node *current = list_head;
    for (;;)
    {
        if (!strcmp(current->signal_name, signal_name))
            return current;
        if (current->next == NULL)
            break;
        current = current->next;
    }
    return NULL;
}


/**
 * Adds new top signal handle to "top_signals_handle" list.
 */
static struct hlist_node* hman_add_handle_to_list(T_PLI_HANDLE handle, const char *signal_name)
{
    pli_printf(PLI_DEBUG, "hman_add_handle_to_list: %s", signal_name);

    /* First entry to empty list */
    if (list_head == NULL)
    {
        list_head = (struct hlist_node *)pli_malloc(sizeof(struct hlist_node));
        list_head->next = NULL;
        list_head->handle = handle; /* Handle allocated from simulator by caller */

        list_head->signal_name = (char *)pli_malloc(strlen(signal_name) + 1);
        if (list_head->signal_name == NULL) {
            pli_printf(PLI_ERROR, "Failed to allocate memory for hlist_node name!");
            return NULL;
        }

        strcpy(list_head->signal_name, signal_name);
        list_head->signal_size = (size_t) PLI_GET(P_PLI_SIZE, handle);

        return list_head;
    }

    struct hlist_node *current = list_head;

    /* Get last entry in the list*/
    while (current->next != NULL)
        current = current->next;

    /* Append entry to the end */
    current->next = (struct hlist_node *)pli_malloc(sizeof(struct hlist_node));
    current = current->next;
    current->next = NULL;
    current->handle = handle;

    current->signal_name = (char *)pli_malloc(strlen(signal_name) + 1);
    if (current->signal_name == NULL) {
        pli_printf(PLI_ERROR, "Failed to allocate memory for hlist_node name!");
        return NULL;
    }

    current->signal_size = (size_t) PLI_GET(P_PLI_SIZE, handle);
    strcpy(current->signal_name, signal_name);

    return current;
}


/******************************************************************************
 * Public API
 *****************************************************************************/

struct hlist_node* hman_get_ctu_vip_net_handle(const char *signal_name)
{
    pli_printf(PLI_DEBUG, "hman_get_ctu_vip_net_handle: %s", signal_name);

    /* Get enry from list (cached handle) */
    struct hlist_node* list_entry = hman_search_handle_list(signal_name);

    /* Not found -> Get from simulator and cache */
    if (list_entry == NULL)
    {
        T_PLI_HANDLE new_signal_handle = hman_create_ctu_vip_signal_handle(signal_name);

        char *full_name;
#if PLI_KIND == PLI_KIND_GHDL_VPI
        full_name = vpi_get_str(vpiFullName, new_signal_handle);
#elif PLI_KIND == PLI_KIND_VCS_VHPI
        full_name = vhpi_get_str(vhpiFullNameP, new_signal_handle);
#elif PLI_KIND == PLI_KIND_NVC_VHPI
        full_name = vhpi_get_str(vhpiFullNameP, new_signal_handle);
#endif
        pli_printf(PLI_DEBUG, "Caching signal handle of: %s", full_name);
        list_entry = hman_add_handle_to_list(new_signal_handle, signal_name);
    }

    return list_entry;
}


void hman_cleanup()
{
    pli_printf(PLI_DEBUG, "hman_cleanup");

    struct hlist_node *current = list_head;
    struct hlist_node *next;

    while (current)
    {
        next = current->next;
        free(current);
        current = next;
    }
}
