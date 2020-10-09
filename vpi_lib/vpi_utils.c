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
#include "vpi_handle_manager.h"


int vpi_drive_str_value(const char *signal_name, const char *value)
{
    struct hlist_node* node = get_top_net_handle(signal_name);

    if (node == NULL)
        return -1;

    char *signal_buffer = malloc(node->signal_size + 1);

    if (signal_buffer == NULL)
        return -1;

    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_value.value.str = signal_buffer;
    memset(signal_buffer, 0, node->signal_size + 1);
    strcpy(signal_buffer, value);

    vpi_put_value(node->handle, &vpi_value, NULL, vpiNoDelay);

    free(signal_buffer);
    return 0;
}


int vpi_read_str_value(const char *signal_name, char *ret_value)
{
    struct hlist_node* node = get_top_net_handle(signal_name);

    if (node == NULL)
        return -1;

    s_vpi_value vpi_value;
    vpi_value.format = vpiBinStrVal;
    vpi_get_value(node->handle, &vpi_value);
    strcpy(ret_value, vpi_value.value.str);

    return 0;
}