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

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "_pli_types.h"
#include "vpi_user.h"
#include "vpi_utils.h"



vpiHandle get_top_net_handle(vpiHandle module_handle, const char *signal_name)
{
    vpiHandle top_scope_h = vpi_handle(vpiScope, module_handle);
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


int vpi_drive_str_value(const char *signal_name, const char *value)
{
    vpiHandle top_iterator = vpi_iterate(vpiModule, NULL);
    vpiHandle top_module = vpi_scan(top_iterator);
    vpiHandle signal_handle = get_top_net_handle(top_module, signal_name);

    if (signal_handle == NULL)
        goto err_signal_handle;

    PLI_INT32 vect_lenght = vpi_get(vpiSize, signal_handle);
    char *signal_buffer = malloc(vect_lenght + 1);

    if (signal_buffer == NULL)
        goto err_alloc_signal_buffer;

    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_value.value.str = signal_buffer;
    memset(signal_buffer, 0, vect_lenght + 1);
    strcpy(signal_buffer, value);

    vpi_put_value(signal_handle, &vpi_value, NULL, vpiNoDelay);

    free(signal_buffer);

err_alloc_signal_buffer:
    vpi_free_object(signal_handle);

err_signal_handle:
    vpi_free_object(top_module);
    vpi_free_object(top_iterator);

    return 0;
}


int vpi_read_str_value(const char *signal_name, char *ret_value)
{
    vpiHandle top_iterator = vpi_iterate(vpiModule, NULL);
    vpiHandle top_module = vpi_scan(top_iterator);
    vpiHandle signal_handle = get_top_net_handle(top_module, signal_name);

    if (signal_handle == NULL)
        return -1;

    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_get_value(signal_handle, &vpi_value);
    strcpy(ret_value, vpi_value.value.str);

    vpi_free_object(signal_handle);
    vpi_free_object(top_module);
    vpi_free_object(top_iterator);

    return 0;
}