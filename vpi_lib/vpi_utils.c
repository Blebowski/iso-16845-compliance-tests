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

pthread_mutex_t handshakeMutex;
pthread_mutexattr_t handshakeMutexAttr;


vpiHandle get_net_handle(vpiHandle moduleHandle, const char *netName)
{
    vpiHandle topScope = vpi_handle(vpiScope, moduleHandle);
    vpiHandle netIterator = vpi_iterate(vpiNet, topScope);
    vpiHandle signalHandle;
    const char *name;

    while ((signalHandle = (vpiHandle)vpi_scan(netIterator)) != NULL)
    {
        name = vpi_get_str(vpiName, signalHandle);
        if (strcmp(name, netName) == 0)
        {
            vpi_free_object(netIterator);
            vpi_free_object(topScope);
            return signalHandle;
        }
    }
    vpi_printf("%s Can't find %s signal", VPI_TAG, netName);
    fprintf(stderr, "%s Can't find %s signal", VPI_TAG, netName);
    vpi_free_object(netIterator);
    vpi_free_object(topScope);
    return NULL;
}


int vpi_drive_str_value(const char *signalName, const char *value)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = get_net_handle(topModule, signalName);

    if (signalHandle == NULL)
        goto err_signal_handle;

    PLI_INT32 vect_lenght = vpi_get(vpiSize, signalHandle);
    char *signal_buffer = malloc(vect_lenght + 1);

    if (signal_buffer == NULL)
        goto err_alloc_signal_buffer;

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpiValue.value.str = signal_buffer;
    memset(signal_buffer, 0, vect_lenght + 1);
    strcpy(signal_buffer, value);

    vpi_put_value(signalHandle, &vpiValue, NULL, vpiNoDelay);

    free(signal_buffer);

err_alloc_signal_buffer:
    vpi_free_object(signalHandle);

err_signal_handle:
    vpi_free_object(topModule);
    vpi_free_object(topIterator);

    return 0;
}


int vpi_read_str_value(const char *signalName, char *retValue)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = get_net_handle(topModule, signalName);

    if (signalHandle == NULL)
        return -1;

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpi_get_value(signalHandle, &vpiValue);
    strcpy(retValue, vpiValue.value.str);

    vpi_free_object(signalHandle);
    vpi_free_object(topModule);
    vpi_free_object(topIterator);

    return 0;
}