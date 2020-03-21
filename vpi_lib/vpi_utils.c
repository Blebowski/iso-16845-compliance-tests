/*
 *  TODO: License
 */

#include <unistd.h>
#include <pthread.h>

#include "_pli_types.h"
#include "vpi_user.h"
#include "vpi_utils.h"

pthread_mutex_t handshakeMutex;
pthread_mutexattr_t handshakeMutexAttr;


void test_lock_handshake_mutex()
{
    //printf("TEST: LOCK\n");
    pthread_mutex_lock(&handshakeMutex);
    //printf("TEST: LOCK OK\n");
}


void test_unlock_handshake_mutex()
{
    //printf("TEST: UNLOCK\n");
    pthread_mutex_unlock(&handshakeMutex);
    //printf("TEST: UNLOCK OK\n");
}

void simulator_lock_handshake_mutex(struct t_cb_data*cb)
{
    //printf("SIMULATOR: LOCK\n");
    pthread_mutex_lock(&handshakeMutex);
    //printf("SIMULATOR: LOCK OK\n");
}


void simulator_unlock_handshake_mutex(struct t_cb_data*cb)
{
    //printf("SIMULATOR: UNLOCK\n");
    pthread_mutex_unlock(&handshakeMutex);
    //printf("SIMULATOR: UNLOCK OK\n");
}


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
            return signalHandle;
    }
    vpi_printf("%s Can't find %s signal", VPI_TAG, netName);
    fprintf(stderr, "%s Can't find %s signal", VPI_TAG, netName);
    return NULL;
}


int vpi_drive_str_value(const char *signalName, char *value)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = get_net_handle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpiValue.value.str = value;
    vpi_put_value(signalHandle, &vpiValue, NULL, vpiNoDelay);
}


int vpi_read_str_value(const char *signalName, char *retValue)
{
    vpiHandle topIterator = vpi_iterate(vpiModule, NULL);
    vpiHandle topModule = vpi_scan(topIterator);
    vpiHandle signalHandle = get_net_handle(topModule, signalName);

    s_vpi_value vpiValue;
    vpiValue.format = vpiBinStrVal;
    vpi_get_value(signalHandle, &vpiValue);
    strcpy(retValue, vpiValue.value.str);
}


int vpi_wait_till_str_value(const char *signalName, char *value)
{
    char readValue[128];
    memset(readValue, 0, sizeof(readValue));
    while (1)
    {
        test_lock_handshake_mutex();
        vpi_read_str_value(signalName, &(readValue[0]));
        test_unlock_handshake_mutex();
        if (strcmp(value, readValue) == 0){
            break;
        }
        usleep(100);
    }
}


void vpi_full_handshake()
{
    vpi_begin_handshake();
    vpi_end_handshake();
}


void vpi_begin_handshake()
{
    test_lock_handshake_mutex();
    vpi_drive_str_value(VPI_SIGNAL_REQ, "1");
    test_unlock_handshake_mutex();
    vpi_wait_till_str_value(VPI_SIGNAL_ACK, "1");
}


void vpi_end_handshake()
{
    test_lock_handshake_mutex();
    vpi_drive_str_value(VPI_SIGNAL_REQ, "0");
    test_unlock_handshake_mutex();
    vpi_wait_till_str_value(VPI_SIGNAL_ACK, "0");
}