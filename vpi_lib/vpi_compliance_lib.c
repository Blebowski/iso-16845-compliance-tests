/**
 * TODO:License
 */

#include "vpi_utils.h"
#include "vpi_compliance_lib.h"
#include <unistd.h>
#include <stdlib.h>

/*****************************************************************************
 * Reset agent functions
 ****************************************************************************/

void reset_agent_assert()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_ASSERT);
    vpi_full_handshake();
}


void reset_agent_deassert()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_DEASSERT);
    vpi_full_handshake();
}


void reset_agent_polarity_set(int polarity)
{
    char pol_str[2];
    sprintf(pol_str, "%d", polarity);
    vpiDriveStrValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_POLARITY_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, pol_str);
    vpi_full_handshake();
}


int reset_agent_polarity_get()
{
    char pol_str[2];
    memset(pol_str, 0, sizeof(pol_str));
    vpiDriveStrValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_POLARITY_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, pol_str);
    vpi_end_handshake();
    return atoi(&(pol_str[0]));
}