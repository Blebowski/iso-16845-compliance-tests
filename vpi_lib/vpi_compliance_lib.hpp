/*
 * TODO:
 */

#include <chrono>

extern "C" {
    #include "vpi_utils.h"
    #include "_pli_types.h"
    #include "vpi_user.h"
}

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

/*
 * Clock generator agent
 */
#define VPI_CLK_AGNT_CMD_START        "00000001"
#define VPI_CLK_AGNT_CMD_STOP         "00000010"
#define VPI_CLK_AGNT_CMD_PERIOD_SET   "00000011"
#define VPI_CLK_AGNT_CMD_PERIOD_GET   "00000100"
#define VPI_CLK_AGNT_CMD_JITTER_SET   "00000101"
#define VPI_CLK_AGNT_CMD_JITTER_GET   "00000110"
#define VPI_CLK_AGNT_CMD_DUTY_SET     "00000111"
#define VPI_CLK_AGNT_CMD_DUTY_GET     "00001000"


/******************************************************************************
 * Reset agent functions
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


/******************************************************************************
 * Clock generator agent functions
 *****************************************************************************/

/*
 *
 */
int clock_agent_start();

/*
 *
 */
int clock_agent_stop();

/*
 *
 */
int clock_agent_set_period(std::chrono::nanoseconds clockPeriod);

/*
 *
 */
std::chrono::nanoseconds clock_agent_get_period();

/*
 *
 */
int clock_agent_set_jitter(std::chrono::nanoseconds clockPeriod);

/*
 *
 */
std::chrono::nanoseconds clock_agent_get_jitter();

/*
 *
 */
int clock_agent_duty_set(int duty);

/*
 *
 */
int clock_agent_duty_get();


#endif