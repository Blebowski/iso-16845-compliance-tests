/*
 *  TODO: License
 */

#include <unistd.h>

#include "_pli_types.h"
#include "vpi_user.h"
#include "vpi_utils.h"


vpiHandle getNetHandle(vpiHandle moduleHandle, const char *netName)
{
    vpiHandle topScope = vpi_handle(vpiScope, moduleHandle);
    vpiHandle netIterator = vpi_iterate(vpiNet, topScope);
    vpiHandle signalHandle;
    const char *name;

    while ((signalHandle = (vpiHandle)vpi_scan(netIterator)) != NULL)
    {
        name = vpi_get_str(vpiName, signalHandle);
        //vpi_printf("Searching net: %s\n", name);
        if (strcmp(name, netName) == 0)
            return signalHandle;
    }
    vpi_printf("%s Can't find %s signal", VPI_TAG, netName);
    fprintf(stderr, "%s Can't find %s signal", VPI_TAG, netName);
    return NULL;
}


int vpiDriveStrValue(const char *signalName, char *value)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = getNetHandle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpiValue.value.str = value;
    vpi_put_value(signalHandle, &vpiValue, NULL, vpiNoDelay);
}


int vpiDriveIntValue(const char* signalName, int value)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = getNetHandle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiIntVal;
    vpiValue.value.integer = value;
    vpi_put_value(signalHandle, &vpiValue, NULL, vpiNoDelay);
}


int vpiReadIntValue(const char *signalName, int *retValue)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = getNetHandle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiIntVal;
    vpi_get_value(signalHandle, &vpiValue);
    *retValue = vpiValue.value.integer;
}


int vpiReadStrValue(const char *signalName, char *retValue)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = getNetHandle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpi_get_value(signalHandle, &vpiValue);

    // TODO: Will this work? Is string passed null terminated??
    memcpy(retValue, vpiValue.value.str, strlen(vpiValue.value.str));
}


int vpiWaitTillStrValue(const char *signalName, char *value)
{
    char readValue[128];
    while (1)
    {
        vpiReadStrValue(signalName, &(readValue[0]));
        if (strcmp(value, readValue) == 0)
            break;
        usleep(10);
    }
}


void vpi_full_handshake()
{
    vpi_begin_handshake();
    vpi_end_handshake();
}


void vpi_begin_handshake()
{
    vpiDriveStrValue(VPI_SIGNAL_REQ, "1");
    vpiWaitTillValue(VPI_SIGNAL_ACK, "1");
}


void vpi_end_handshake()
{
    vpiDriveStrValue(VPI_SIGNAL_REQ, "0");
    vpiWaitTillValue(VPI_SIGNAL_ACK, "0");
}