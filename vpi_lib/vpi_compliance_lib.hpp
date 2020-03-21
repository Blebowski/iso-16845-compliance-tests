/*
 * TODO:
 */

#include <chrono>
#include <atomic>

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
#define VPI_DEST_TEST_CONTROLLER_AGENT (char*)"00000000"
#define VPI_DEST_CLK_GEN_AGENT         (char*)"00000001"
#define VPI_DEST_RES_GEN_AGENT         (char*)"00000010"
#define VPI_DEST_MEM_BUS_AGENT         (char*)"00000011"
#define VPI_DEST_CAN_AGENT             (char*)"00000100"

/*
 * Reset agent
 */
#define VPI_RST_AGNT_CMD_ASSERT        (char*)"00000001"
#define VPI_RST_AGNT_CMD_DEASSERT      (char*)"00000010"
#define VPI_RST_AGNT_CMD_POLARITY_SET  (char*)"00000011"
#define VPI_RST_AGNT_CMD_POLARITY_GET  (char*)"00000100"

/*
 * Clock generator agent
 */
#define VPI_CLK_AGNT_CMD_START        (char*)"00000001"
#define VPI_CLK_AGNT_CMD_STOP         (char*)"00000010"
#define VPI_CLK_AGNT_CMD_PERIOD_SET   (char*)"00000011"
#define VPI_CLK_AGNT_CMD_PERIOD_GET   (char*)"00000100"
#define VPI_CLK_AGNT_CMD_JITTER_SET   (char*)"00000101"
#define VPI_CLK_AGNT_CMD_JITTER_GET   (char*)"00000110"
#define VPI_CLK_AGNT_CMD_DUTY_SET     (char*)"00000111"
#define VPI_CLK_AGNT_CMD_DUTY_GET     (char*)"00001000"

/*
 * Memory bus agent
 */
#define VPI_MEM_BUS_AGNT_START             (char*)"00000001"
#define VPI_MEM_BUS_AGNT_STOP              (char*)"00000010"
#define VPI_MEM_BUS_AGNT_WRITE             (char*)"00000011"
#define VPI_MEM_BUS_AGNT_READ              (char*)"00000100"
#define VPI_MEM_BUS_AGNT_X_MODE_START      (char*)"00000101"
#define VPI_MEM_BUS_AGNT_X_MODE_STOP       (char*)"00000110"
#define VPI_MEM_BUS_AGNT_SET_X_MODE_SETUP  (char*)"00000111"
#define VPI_MEM_BUS_AGNT_SET_X_MODE_HOLD   (char*)"00001000"
#define VPI_MEM_BUS_AGNT_SET_PERIOD        (char*)"00001001"
#define VPI_MEM_BUS_AGNT_SET_OUTPUT_DELAY  (char*)"00001010"
#define VPI_MEM_BUS_AGNT_WAIT_DONE         (char*)"00001011"


#define VPI_CAN_AGNT_DRIVER_START                  (char*)"00000001"
#define VPI_CAN_AGNT_DRIVER_STOP                   (char*)"00000010"
#define VPI_CAN_AGNT_DRIVER_FLUSH                  (char*)"00000011"
#define VPI_CAN_AGNT_DRIVER_GET_PROGRESS           (char*)"00000100"
#define VPI_CAN_AGNT_DRIVER_GET_DRIVEN_VAL         (char*)"00000101"
#define VPI_CAN_AGNT_DRIVER_PUSH_ITEM              (char*)"00000110"
#define VPI_CAN_AGNT_DRIVER_SET_WAIT_TIMEOUT       (char*)"00000111"
#define VPI_CAN_AGNT_DRIVER_WAIT_FINISH            (char*)"00001000"
#define VPI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM      (char*)"00001001"
#define VPI_CAN_AGNT_DRIVER_DRIVE_ALL_ITEM         (char*)"00001010"

#define VPI_CAN_AGNT_MONITOR_START                 (char*)"00001011"
#define VPI_CAN_AGNT_MONITOR_STOP                  (char*)"00001100"
#define VPI_CAN_AGNT_MONITOR_FLUSH                 (char*)"00001101"
#define VPI_CAN_AGNT_MONITOR_GET_STATE             (char*)"00001110"
#define VPI_CAN_AGNT_MONITOR_GET_MONITORED_VAL     (char*)"00001111"
#define VPI_CAN_AGNT_MONITOR_PUSH_ITEM             (char*)"00010000"
#define VPI_CAN_AGNT_MONITOR_SET_WAIT_TIMEOUT      (char*)"00010001"
#define VPI_CAN_AGNT_MONITOR_WAIT_FINISH           (char*)"00010010"
#define VPI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM   (char*)"00010011"
#define VPI_CAN_AGNT_MONITOR_MONITOR_ALL_ITEMS     (char*)"00010100"

#define VPI_CAN_AGNT_MONITOR_SET_TRIGGER           (char*)"00010101"
#define VPI_CAN_AGNT_MONITOR_GET_TRIGGER           (char*)"00010110"

#define VPI_CAN_AGNT_MONITOR_SET_SAMPLE_RATE       (char*)"00010111"
#define VPI_CAN_AGNT_MONITOR_GET_SAMPLE_RATE       (char*)"00011000"

#define VPI_CAN_AGNT_MONITOR_CHECK_RESULT          (char*)"00011001"

enum CanAgentMonitorState
{
    CAN_AGENT_MONITOR_DISABLED,
    CAN_AGENT_MONITOR_WAITING_FOR_TRIGGER,
    CAN_AGENT_MONITOR_RUNNING,
    CAN_AGENT_MONITOR_PASSED,
    CAN_AGENT_MONITOR_FAILED
};

enum CanAgentMonitorTrigger
{
     CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY,
     CAN_AGENT_MONITOR_TRIGGER_RX_RISING,
     CAN_AGENT_MONITOR_TRIGGER_RX_FALLING,
     CAN_AGENT_MONITOR_TRIGGER_TX_RISING,
     CAN_AGENT_MONITOR_TRIGGER_TX_FALLING,
     CAN_AGENT_MONITOR_TRIGGER_TIME_ELAPSED,
     CAN_AGENT_MONITOR_TRIGGER_DRIVER_START,
     CAN_AGENT_MONITOR_TRIGGER_DRIVER_STOP
};


/******************************************************************************
 * Reset agent functions
 *****************************************************************************/

/*
 *
 */
void resetAgentAssert();

/*
 *
 */
void resetAgentDeassert();

/*
 *
 */
void resetAgentPolaritySet(int polarity);

/*
 *
 */
int resetAgentPolarityGet();


/******************************************************************************
 * Clock generator agent functions
 *****************************************************************************/

/*
 *
 */
int clockAgentStart();

/*
 *
 */
int clockAgentStop();

/*
 *
 */
int clockAgentSetPeriod(std::chrono::nanoseconds clockPeriod);

/*
 *
 */
std::chrono::nanoseconds clockAgentGetPeriod();

/*
 *
 */
int clockAgentSetJitter(std::chrono::nanoseconds clockPeriod);

/*
 *
 */
std::chrono::nanoseconds clockAgentGetJitter();

/*
 *
 */
int clockAgentSetDuty(int duty);

/*
 *
 */
int clockAgentGetDuty();


/******************************************************************************
 * Memory bus agent functions
 *****************************************************************************/

/*
 *
 */
void memBusAgentStart();

/*
 *
 */
void memBusAgentStop();

/*
 *
 */
void memBusAgentWrite32(int address, uint32_t data);

/*
 *
 */
void memBusAgentWrite16(int address, uint16_t data);

/*
 *
 */
void memBusAgentWrite8(int address, uint8_t data);

/*
 *
 */
uint32_t memBusAgentRead32(int address);

/*
 *
 */
uint16_t memBusAgentRead16(int address);

/*
 *
 */
uint8_t memBusAgentRead8(int address);

/*
 *
 */
void memBusAgentXModeStart();

/*
 *
 */
void memBusAgentXModeStop();

/*
 *
 */
void memBusAgentSetXModeSetup(std::chrono::nanoseconds setup);

/*
 *
 */
void memBusAgentSetXModeHold(std::chrono::nanoseconds hold);

/*
 *
 */
void memBusAgentSetPeriod(std::chrono::nanoseconds period);

/*
 *
 */
void memBusAgentSetOutputDelay(std::chrono::nanoseconds delay);


/******************************************************************************
 * CAN Agent functions
 *****************************************************************************/

/*
 *
 */
void canAgentDriverStart();

/*
 *
 */
void canAgentDriverStop();

/*
 *
 */
void canAgentDriverFlush();

/*
 *
 */
bool canAgentDriverGetProgress();

/*
 *
 */
char canAgentDriverGetDrivenVal();

/*
 *
 */
void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration);

/*
 *
 */
void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg);

/*
 *
 */
void canAgentDriverSetWaitTimeout(std::chrono::nanoseconds timeout);

/*
 *
 */
void canAgentDriverWaitFinish();

/*
 *
 */
void canAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg);

/*
 *
 */
void canAgentDriveSingleItem(char vdrivenValuealue, std::chrono::nanoseconds duration);

/*
 *
 */
void canAgentDriveAllItems();

/*
 *
 */
void canAgentDriveAllItems();

/*
 *
 */
void canAgentMonitorStart();

/*
 *
 */
void canAgentMonitorStop();

/*
 *
 */
void canAgentMonitorFlush();

/*
 *
 */
CanAgentMonitorState canAgentMonitorGetState();

/*
 *
 */
char canAgentMonitorGetMonitoredVal();

/*
 *
 */
void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration);

/*
 *
 */
void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration, std::string msg);

/*
 *
 */
void canAgentMonitorSetWaitTimeout(std::chrono::nanoseconds timeout);

/*
 *
 */
void canAgentMonitorWaitFinish();

/*
 *
 */
void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration);

/*
 *
 */
void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration, std::string msg);

/*
 *
 */
void canAgentMonitorAllItems();

/*
 *
 */
void canAgentMonitorSetTrigger(CanAgentMonitorTrigger trigger);

/*
 *
 */
CanAgentMonitorTrigger canAgentMonitorGetTrigger();

/*
 *
 */
void canAgentMonitorSetSampleRate(std::chrono::nanoseconds sampleRate);

/*
 *
 */
std::chrono::nanoseconds canAgentMonitorgetSampleRate();

/*
 *
 */
void canAgentCheckResult();


/******************************************************************************
 * Test controller agent functions
 *****************************************************************************/

/*
 *
 */
void testControllerAgentEndTest(bool success);

#endif