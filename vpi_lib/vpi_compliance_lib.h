/*
 * TODO:
 */

#include "_pli_types.h"
#include "vpi_user.h"
#include "vpi_utils.h"

#ifndef VPI_COMPLIANCE_LIB
#define VPI_COMPLIANCE_LIB

/*
 * Agent destinations with testbench
 */
#define VPI_DEST_TEST_CONTROLLER_AGENT "00000000"
#define VPI_DEST_CLK_GEN_AGENT         "00000001"
#define VPI_DEST_RES_GEN_AGENT         "00000010"
#define VPI_DEST_MEM_BUS_AGENT         "00000011"
#define VPI_DEST_CAN_AGENT             "00000100"

/*
 * Reset agent
 */
#define VPI_RST_AGNT_CMD_ASSERT        "00000001"
#define VPI_RST_AGNT_CMD_DEASSERT      "00000010"
#define VPI_RST_AGNT_CMD_POLARITY_SET  "00000011"
#define VPI_RST_AGNT_CMD_POLARITY_GET  "00000100"



/******************************************************************************
 * Function prototypes
 *****************************************************************************/

/*
 *
 */
void reset_agent_assert();

/*
 *
 */
void reset_agent_deassert();

/*
 *
 */
void reset_agent_polarity_set(int polarity);

/*
 *
 */
int reset_agent_polarity_get();

#endif