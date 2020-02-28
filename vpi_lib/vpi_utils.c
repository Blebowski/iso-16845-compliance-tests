/*
 *  TODO: License
 */

#include <unistd.h>
#include <pthread.h>

#include "_pli_types.h"
#include "vpi_user.h"
#include "vpi_utils.h"

pthread_mutex_t handshakeMutex;


void lockHandshakeMutex()
{
    pthread_mutex_lock(&handshakeMutex);
}


void unlockHandshakeMutex()
{
    pthread_mutex_unlock(&handshakeMutex);
}


vpiHandle getNetHandle(vpiHandle moduleHandle, const char *netName)
{
    vpiHandle topScope = vpi_handle(vpiScope, moduleHandle);
    vpiHandle netIterator = vpi_iterate(vpiNet, topScope);
    vpiHandle signalHandle;
    const char *name;

    while ((signalHandle = (vpiHandle)vpi_scan(netIterator)) != NULL)
    {
        name = vpi_get_str(vpiName, signalHandle);
        if (strcmp(name, netName) == 0)
            return signalHandle;
    }
    vpi_printf("%s Can't find %s signal", VPI_TAG, netName);
    fprintf(stderr, "%s Can't find %s signal", VPI_TAG, netName);
    return NULL;
}


int vpiDriveStrValue(const char *signalName, char *value)
{
    lockHandshakeMutex();

    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = getNetHandle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpiValue.value.str = value;
    vpi_put_value(signalHandle, &vpiValue, NULL, vpiNoDelay);

    unlockHandshakeMutex();
}


int vpiReadStrValue(const char *signalName, char *retValue)
{
    lockHandshakeMutex();

    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = getNetHandle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpi_get_value(signalHandle, &vpiValue);

    // TODO: Will this work? Is string passed null terminated??
    memcpy(retValue, vpiValue.value.str, strlen(vpiValue.value.str));

    unlockHandshakeMutex();
}


int vpiWaitTillStrValue(const char *signalName, char *value)
{
    char readValue[128];
    memset(readValue, 0, sizeof(readValue));
    while (1)
    {
        vpiReadStrValue(signalName, &(readValue[0]));
        if (strcmp(value, readValue) == 0){
            break;
        }
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
    vpiWaitTillStrValue(VPI_SIGNAL_ACK, "1");
}


void vpi_end_handshake()
{
    vpiDriveStrValue(VPI_SIGNAL_REQ, "0");
    vpiWaitTillStrValue(VPI_SIGNAL_ACK, "0");
}