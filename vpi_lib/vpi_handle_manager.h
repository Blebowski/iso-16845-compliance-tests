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
 * Manages VPI handles, so that they are not queried over VPI multiple times!
 * This avoids memory leaks inside GHDL!
 *  
 *****************************************************************************/

#include "vpi_user.h"


struct hlist_node {
    vpiHandle handle;
    char *signal_name;
    int signal_size;
    struct hlist_node *next;
};

/**
 * 
 */
vpiHandle get_top_module_handle();


/**
 * 
 */
struct hlist_node* get_top_net_handle(const char *signal_name);