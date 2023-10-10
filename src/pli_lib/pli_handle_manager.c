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

#include "pli_handle_manager.h"


/* Handle to CTU CAN FD VIP - Test controller agent module */
static vpiHandle ctu_vip_handle = NULL;

/* List of handles which has been already queried by library */
static struct hlist_node *list_head = NULL;

/* Hierarchical path in HDL simulator where CTU CAN FD VIP is instantiated */
static char ctu_vip_path[] = CTU_VIP_HIERARCHICAL_PATH;

/**
 * @brief Searches through module instances recursively if module matches
 *        the name.
 * @param module Handle to module to be searched
 * @param exp_name Expected name of the module
 */
static void hman_search_ctu_vip_handle(vpiHandle module, char *exp_name)
{
    //fprintf(stderr, "%s Searching: %s Expected: %s\n", PLI_TAG, vpi_get_str(vpiName, module), exp_name);

    if (!strcmp(exp_name, vpi_get_str(vpiName, module)))
    {
        vpiHandle mod_it = vpi_iterate(vpiModule, module);
        vpiHandle mod_tmp;
        char *name = strtok(NULL, "/");

        /* We descend to VIP instance, we have no more paths */
        if (name == NULL)
        {
            ctu_vip_handle = module;
            return;
        }

        while ((mod_tmp = (vpiHandle)vpi_scan(mod_it)) != NULL)
            hman_search_ctu_vip_handle(mod_tmp, name);
    }
}


/**
 * @returns Handle to CTU CAN FD VIP module inside HDL simulation.
 */
static vpiHandle hman_get_ctu_vip_handle()
{
    /* Search upon first request and cache the handle */
    if (ctu_vip_handle == NULL)
    {
#ifdef DEBUG_BUILD
        vpi_printf("%s Searching for CTU CAN FD VIP module: %s\n", PLI_TAG, CTU_VIP_HIERARCHICAL_PATH);
#endif
        char *top_name = strtok(ctu_vip_path, "/");

        /* Finds the handle and assigns to "ctu_vip_handle" */
        vpiHandle top_mod_it = vpi_iterate(vpiModule, NULL);
        vpiHandle top_mod_h = vpi_scan(top_mod_it);
        hman_search_ctu_vip_handle(top_mod_h, top_name);
        vpi_free_object(top_mod_it);
    }

#ifdef DEBUG_BUILD
    vpi_printf("Found CTU CAN FD VIP is: %s\n", vpi_get_str(vpiFullName, ctu_vip_handle));
#endif
    return ctu_vip_handle;
}


/**
 * @brief Creates handle to signal in CTU CAN FD VIP.
 * @param Signal_name name whose handle to create.
 * @returns VPI handle to signal.
 */
static vpiHandle hman_create_ctu_vip_handle(const char *signal_name)
{
    vpiHandle ctu_scope_h = vpi_handle(vpiScope, hman_get_ctu_vip_handle());
    vpiHandle net_iterator = vpi_iterate(vpiNet, ctu_scope_h);
    vpiHandle signal_handle;
    const char *name;

    while ((signal_handle = (vpiHandle)vpi_scan(net_iterator)) != NULL)
    {
        name = vpi_get_str(vpiName, signal_handle);
        if (strcmp(name, signal_name) == 0)
        {
            vpi_free_object(net_iterator);
            vpi_free_object(ctu_scope_h);
            return signal_handle;
        }
    }

    vpi_printf("%s Can't find %s signal", PLI_TAG, signal_name);
    fprintf(stderr, "%s Can't find %s signal", PLI_TAG, signal_name);
    vpi_free_object(net_iterator);
    vpi_free_object(ctu_scope_h);
    return NULL;
}


/**
 * @brief Searches handle list for handle with matching signal name.
 * @param signal_name Name of signal whose handle to search for
 * @param Pointer to handle node in Handle list.
 */
static struct hlist_node* hman_search_handle_list(const char *signal_name)
{
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
 * TODO: Handle malloc return code properly!
 */
static struct hlist_node* hman_add_handle_to_list(vpiHandle handle, const char *signal_name)
{
    /* First entry to empty list */
    if (list_head == NULL)
    {
        list_head = (struct hlist_node *)malloc(sizeof(struct hlist_node));
        list_head->next = NULL;
        list_head->handle = handle; /* Handle allocated from simulator by caller */
        list_head->signal_name = (char *) malloc(strlen(signal_name) + 1);
        list_head->signal_size = vpi_get(vpiSize, handle);
        strcpy(list_head->signal_name, signal_name);
        return list_head;
    }

    struct hlist_node *current = list_head;

    /* Get last entry in the list*/
    while (current->next != NULL)
        current = current->next;

    /* Append entry to the end */
    current->next = (struct hlist_node *)malloc(sizeof(struct hlist_node));
    current = current->next;
    current->next = NULL;
    current->handle = handle;
    current->signal_name = (char *)malloc(strlen(signal_name) + 1);
    current->signal_size = vpi_get(vpiSize, handle);
    strcpy(current->signal_name, signal_name);

    return current;
}


/******************************************************************************
 * Public API
 *****************************************************************************/

struct hlist_node* hman_get_ctu_vip_net_handle(const char *signal_name)
{
    /* Get enry from list (cached handle) */
    struct hlist_node* list_entry = hman_search_handle_list(signal_name);

    /* Not found -> Get from simulator and cache */
    if (list_entry == NULL)
    {
        vpiHandle new_signal_handle = hman_create_ctu_vip_handle(signal_name);
#ifdef DEBUG_BUILD
        vpi_printf("%s Caching signal handle of: %s\n", PLI_TAG,
                    vpi_get_str(vpiFullName, new_signal_handle));
#endif
        list_entry = hman_add_handle_to_list(new_signal_handle, signal_name);
    }

    return list_entry;
}


void hman_cleanup()
{
    vpi_printf("%s Handle manager cleanup\n", PLI_TAG);

    struct hlist_node *current = list_head;
    struct hlist_node *next;

    while (current)
    {
        next = current->next;
        free(current);
        current = next;
    }
}
