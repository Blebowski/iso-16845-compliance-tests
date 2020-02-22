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
#define VPI_DEST_TEST_CONTROLLER_AGENT 0
#define VPI_DEST_CLK_GEN_AGENT         1
#define VPI_DEST_RES_GEN_AGENT         2
#define VPI_DEST_MEM_BUS_AGENT         3
#define VPI_DEST_CAN_AGENT             4

/*
 * Reset agent
 */
#define VPI_RST_AGNT_CMD_ASSERT        0
#define VPI_RST_AGNT_CMD_DEASSERT      1
#define VPI_RST_AGNT_CMD_POLARITY_SET  2
#define VPI_RST_AGNT_CMD_POLARITY_GET  3



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
void reset_agent_dessert();

/*
 *
 */
void reset_agent_polarity_set(char *polarity);

/*
 *
 */
void reset_agent_polarity_get(char *polarity);


#endif