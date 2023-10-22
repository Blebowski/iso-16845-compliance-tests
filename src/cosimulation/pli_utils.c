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
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "pli_utils.h"
#include "pli_handle_manager.h"

// Global message print severity level
static t_pli_msg_severity pli_severity_level = PLI_INFO;


#if PLI_KIND == PLI_KIND_VCS_VHPI

static char raw_to_std_logic_char(char raw) {
    switch (raw) {
    case 0x0: return 'U';
    case 0x1: return 'X';
    case 0x2: return '0';
    case 0x3: return '1';
    case 0x4: return 'Z';
    case 0x5: return 'W';
    case 0x6: return 'L';
    case 0x7: return 'H';
    case 0x8: return '-';
    default:  return 0;
    }
}

static char std_logic_char_to_raw(char std_logic) {
    switch (std_logic) {
    case 'U': return 0x0;
    case 'X': return 0x1;
    case '0': return 0x2;
    case '1': return 0x3;
    case 'Z': return 0x4;
    case 'W': return 0x5;
    case 'L': return 0x6;
    case 'H': return 0x7;
    case '-': return 0x8;
    default:  return 0;
    }
}

#endif


int pli_drive_str_value(const char *signal_name, const char *value)
{
    pli_printf(PLI_DEBUG, "pli_drive_str_value: %s = %s\n", signal_name, value);

    struct hlist_node* node = hman_get_ctu_vip_net_handle(signal_name);

    if (node == NULL)
        return -1;

#if PLI_KIND == PLI_KIND_GHDL_VPI
    char *signal_buffer = pli_malloc(node->signal_size + 1);

    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_value.value.str = signal_buffer;

    memset(signal_buffer, 0, node->signal_size + 1);
    strcpy(signal_buffer, value);

    vpi_put_value(node->handle, &vpi_value, NULL, vpiNoDelay);
    free(signal_buffer);

#elif PLI_KIND == PLI_KIND_VCS_VHPI

    int len = node->signal_size;

    // Allocate Buffers
    unsigned *tmp = pli_malloc(sizeof(unsigned) * len);
    unsigned *tmp2 = pli_malloc(sizeof(unsigned) * len);
    for (int i = 0; i < len; i++) {
        tmp[i] = 0x2;
        tmp2[i] = 0x2;
    }

    // Convert std_logic string to VCS raw format
    for (size_t i = 0; i < strlen(value); i++)
        tmp[i] = (unsigned)std_logic_char_to_raw(value[i]);

    // Flip bits
    if (len > 1) {
        for (int i = 0; i < len; i += 8)
            tmp2[i] = tmp[len - i - 1];
    } else {
        tmp2[0] = tmp[0];
    }

    vhpiValueT vhpi_value;
    vhpi_value.bufSize = sizeof(unsigned) * len;

    if (len == 1) {
        vhpi_value.format = vhpiEnumVal;
        vhpi_value.value.enumval = tmp2[0];
    } else {
        vhpi_value.format = vhpiEnumVecVal;
        vhpi_value.value.enums = tmp;
    }

    vhpi_put_value(node->handle, &vhpi_value, vhpiForcePropagate);

    free(tmp);
    free(tmp2);

#endif

    return 0;
}


int pli_read_str_value(const char *signal_name, char *ret_value)
{
    pli_printf(PLI_DEBUG, "pli_read_str_value: %s Entered \n", signal_name);

    struct hlist_node* node = hman_get_ctu_vip_net_handle(signal_name);

    if (node == NULL)
        return -1;

#if PLI_KIND == PLI_KIND_GHDL_VPI
    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_get_value(node->handle, &vpi_value);
    strcpy(ret_value, vpi_value.value.str);

#elif PLI_KIND == PLI_KIND_VCS_VHPI

    int len = node->signal_size;

    vhpiValueT vhpi_value;
    vhpi_value.bufSize = len;
    vhpi_value.format = vhpiRawData;

    vhpi_get_value(node->handle, &vhpi_value);

    // Convert VCS raw to std_logic string
    char *tmp = pli_malloc(len + 1);
    memset(tmp, 0, len + 1);
    for (int i = 0; i < len; i++) {
        char *bit = (char*)(vhpi_value.value.ptr) + i;
        tmp[i] = raw_to_std_logic_char(*bit);
    }

    // Flip bit order word.
    char *tmp2 = pli_malloc(len + 1);
    memset(tmp2, 0, len + 1);
    for (int i = 0; i < len; i ++)
        tmp2[i] = tmp[len - i - 1];

    // Caller must satisfy sufficient length of ret_value buffer
    memcpy(ret_value, tmp2, len);

    free(tmp);
    free(tmp2);

#endif

    pli_printf(PLI_DEBUG, "pli_read_str_value: %s Returns: %s \n", signal_name, ret_value);

    return 0;
}


T_PLI_HANDLE pli_register_cb(T_PLI_REASON reason, T_PLI_HANDLE handle, void (*cb_fnc)(T_PLI_CB_ARGS))
{
    pli_printf(PLI_DEBUG, "pli_register_cb: reason: %d, handle: %p, cb_fnc: %p \n",
                reason, handle, cb_fnc);

#if PLI_KIND == PLI_KIND_GHDL_VPI
    s_cb_data cb;

    cb.reason = reason;
    cb.cb_rtn = (PLI_INT32 (*)(struct t_cb_data*cb))(cb_fnc);
    cb.user_data = NULL;

    if (reason == cbValueChange) {
        s_vpi_time vpi_clk_time_data = {
            .type = vpiSimTime,
            .high = 0,
            .low = 0,
            .real = 0.0
        };

        s_vpi_value vpi_clk_value_data = {
            .format = vpiBinStrVal
        };

        cb.time = &vpi_clk_time_data;
        cb.value = &vpi_clk_value_data;
        cb.obj = handle;
    }

    return vpi_register_cb(&cb);

#elif PLI_KIND == PLI_KIND_VCS_VHPI

    static vhpiCbDataT cb;
    static vhpiValueT value;
    static vhpiTimeT time;

    value.format = vhpiEnumVal;
    value.bufSize = 0;
    value.value.intgs = 0;

    cb.reason = reason;
    cb.cbf = cb_fnc;
    cb.time = &time;
    cb.value = &value;
    cb.obj = handle;

    // TODO: Figure out why VCS returns NULL handle despite correctly registering the handle!
    return vhpi_register_cb(&cb, 0);
#endif

}

void pli_printf(t_pli_msg_severity severity, const char *fmt, ...)
{
    if (severity >= pli_severity_level) {
        va_list args;
        va_start(args, fmt);

        char tmp[2048];
        vsnprintf(tmp, 2048, fmt, args);

#if PLI_KIND == PLI_KIND_GHDL_VPI
        vpi_printf("%s ", PLI_TAG);
        vpi_printf(tmp);
#elif PLI_KIND == PLI_KIND_VCS_VHPI
        vhpi_printf("%s ", PLI_TAG);
        vhpi_printf(tmp);
#endif
        va_end(args);
    }
}

void* pli_malloc(size_t size)
{
    pli_printf(PLI_DEBUG, "pli_malloc: size=%d\n", size);

    void *p = malloc(size);

    if (p == NULL) {
        pli_printf(PLI_ERROR, "malloc failed for size of: %d\n", size);
        pli_printf(PLI_ERROR, "Can't continue, exiting application...\n", size);
        exit(1);
    }

    return p;
}

void pli_print_handle(T_PLI_HANDLE handle)
{
    pli_printf(PLI_INFO, "HANDLE: %p\n", handle);
}


