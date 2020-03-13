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

/*
 * Memory bus agent
 */
#define VPI_MEM_BUS_AGNT_START             "00000001"
#define VPI_MEM_BUS_AGNT_STOP              "00000010"
#define VPI_MEM_BUS_AGNT_WRITE             "00000011"
#define VPI_MEM_BUS_AGNT_READ              "00000100"
#define VPI_MEM_BUS_AGNT_X_MODE_START      "00000101"
#define VPI_MEM_BUS_AGNT_X_MODE_STOP       "00000110"
#define VPI_MEM_BUS_AGNT_SET_X_MODE_SETUP  "00000111"
#define VPI_MEM_BUS_AGNT_SET_X_MODE_HOLD   "00001000"
#define VPI_MEM_BUS_AGNT_SET_PERIOD        "00001001"
#define VPI_MEM_BUS_AGNT_SET_OUTPUT_DELAY  "00001010"
#define VPI_MEM_BUS_AGNT_WAIT_DONE         "00001011"


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


/******************************************************************************
 * Memory bus agent functions
 *****************************************************************************/

/*
 *
 */
void mem_bus_agent_start();

/*
 *
 */
void mem_bus_agent_stop();

/*
 *
 */
void mem_bus_agent_write32(int address, uint32_t data);

/*
 *
 */
void mem_bus_agent_write16(int address, uint16_t data);

/*
 *
 */
void mem_bus_agent_write8(int address, uint8_t data);

/*
 *
 */
uint32_t mem_bus_agent_read32(int address);

/*
 *
 */
uint16_t mem_bus_agent_read16(int address);

/*
 *
 */
uint8_t mem_bus_agent_read8(int address);

/*
 *
 */
void mem_bus_agent_x_mode_start();

/*
 *
 */
void mem_bus_agent_x_mode_stop();

/*
 *
 */
void mem_bus_agent_set_x_mode_setup(std::chrono::nanoseconds setup);

/*
 *
 */
void mem_bus_agent_set_x_mode_hold(std::chrono::nanoseconds hold);

/*
 *
 */
void mem_bus_agent_set_period(std::chrono::nanoseconds period);

/*
 *
 */
void mem_bus_agent_set_output_delay(std::chrono::nanoseconds delay);


#endif