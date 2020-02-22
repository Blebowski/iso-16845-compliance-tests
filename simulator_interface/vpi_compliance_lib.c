/**
 * TODO:License
 */

#include "vpi_utils.h"
#include "vpi_compliance_lib.h"


/*****************************************************************************
 * Reset agent functions
 ****************************************************************************/

void reset_agent_assert()
{
    vpiDriveIntValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveIntValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_ASSERT);
    vpi_full_handshake();
}


void reset_agent_dessert()
{
    vpiDriveIntValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveIntValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_DEASSERT);
    vpi_full_handshake();
}


void reset_agent_polarity_set(char *polarity)
{
    vpiDriveIntValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveIntValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_POLARITY_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, polarity);
    vpi_full_handshake();
}


void reset_agent_polarity_get(char *polarity)
{
    vpiDriveIntValue(VPI_SIGNAL_DEST, VPI_DEST_RES_GEN_AGENT);
    vpiDriveIntValue(VPI_SIGNAL_CMD, VPI_RST_AGNT_CMD_POLARITY_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, polarity);
    vpi_end_handshake();
}