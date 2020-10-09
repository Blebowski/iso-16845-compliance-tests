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

#include "_pli_types.h"
#include "vpi_user.h"
#include "vpi_utils.h"
#include "vpi_handle_manager.h"


static vpiHandle top_module_handle = NULL;
static struct hlist_node *list_head = NULL;


/**
 * Creates handle to top level signal by query to simulator via VPI.
 */
vpiHandle create_top_net_handle(const char *signal_name)
{
    vpiHandle top_scope_h = vpi_handle(vpiScope, get_top_module_handle());
    vpiHandle net_iterator = vpi_iterate(vpiNet, top_scope_h);
    vpiHandle signal_handle;
    const char *name;

    while ((signal_handle = (vpiHandle)vpi_scan(net_iterator)) != NULL)
    {
        name = vpi_get_str(vpiName, signal_handle);
        if (strcmp(name, signal_name) == 0)
        {
            vpi_free_object(net_iterator);
            vpi_free_object(top_scope_h);
            return signal_handle;
        }
    }

    vpi_printf("%s Can't find %s signal", VPI_TAG, signal_name);
    fprintf(stderr, "%s Can't find %s signal", VPI_TAG, signal_name);
    vpi_free_object(net_iterator);
    vpi_free_object(top_scope_h);
    return NULL;
}


/**
 * Searches "top_signals_handle_list" for handle of required name.
 */
struct hlist_node* search_top_handle_list(const char *signal_name)
{
    if (list_head == NULL)
        return NULL;

    struct hlist_node *current = list_head;
    for (;;)
    {
        if (!strcmp(current->signal_name, signal_name))
            return current;
        if (current->next == NULL)
            return NULL;
        current = current->next;
    }
    return NULL;
}


/**
 * Adds new top signal handle to "top_signals_handle" list.
 * TODO: Handle malloc return code properly!
 */
struct hlist_node* add_handle_to_list(vpiHandle handle, const char *signal_name)
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

/**
 * If top handle is buffered, return it. Search for it otherwise.
 */
vpiHandle get_top_module_handle()
{
    if (top_module_handle == NULL)
    {
        vpiHandle top_module_iterator = vpi_iterate(vpiModule, NULL);
        top_module_handle = vpi_scan(top_module_iterator);
        vpi_free_object(top_module_iterator);
    }
    return top_module_handle;
}


/**
 * Search top handle list if handle is present there. If yes, return it. If not
 * then request it from simulator and add to the list!
 */
struct hlist_node* get_top_net_handle(const char *signal_name)
{
    struct hlist_node* list_entry = search_top_handle_list(signal_name);
    if (list_entry == NULL)
    {
        vpiHandle new_signal_handle = create_top_net_handle(signal_name);
        list_entry = add_handle_to_list(new_signal_handle, signal_name);
    }
    return list_entry;
}
