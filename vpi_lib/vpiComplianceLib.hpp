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

#include <chrono>
#include <atomic>

extern "C" {
    #include "vpi_utils.h"
    #include "_pli_types.h"
    #include "vpi_user.h"
}

#ifndef VPI_COMPLIANCE_LIB
#define VPI_COMPLIANCE_LIB


/******************************************************************************
 * @section Type/Macros definition
 *****************************************************************************/

/**
 * @subsection Agent destinations with testbench
 */
#define VPI_DEST_TEST_CONTROLLER_AGENT (char*)"00000000"
#define VPI_DEST_CLK_GEN_AGENT         (char*)"00000001"
#define VPI_DEST_RES_GEN_AGENT         (char*)"00000010"
#define VPI_DEST_MEM_BUS_AGENT         (char*)"00000011"
#define VPI_DEST_CAN_AGENT             (char*)"00000100"

/**
 * @subsection Reset agent
 */
#define VPI_RST_AGNT_CMD_ASSERT        (char*)"00000001"
#define VPI_RST_AGNT_CMD_DEASSERT      (char*)"00000010"
#define VPI_RST_AGNT_CMD_POLARITY_SET  (char*)"00000011"
#define VPI_RST_AGNT_CMD_POLARITY_GET  (char*)"00000100"

/**
 * @subsection Clock generator agent
 */
#define VPI_CLK_AGNT_CMD_START        (char*)"00000001"
#define VPI_CLK_AGNT_CMD_STOP         (char*)"00000010"
#define VPI_CLK_AGNT_CMD_PERIOD_SET   (char*)"00000011"
#define VPI_CLK_AGNT_CMD_PERIOD_GET   (char*)"00000100"
#define VPI_CLK_AGNT_CMD_JITTER_SET   (char*)"00000101"
#define VPI_CLK_AGNT_CMD_JITTER_GET   (char*)"00000110"
#define VPI_CLK_AGNT_CMD_DUTY_SET     (char*)"00000111"
#define VPI_CLK_AGNT_CMD_DUTY_GET     (char*)"00001000"

/**
 * @subsection Memory bus agent
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

/**
 * @subsection CAN agent
 */
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

#define VPI_CAN_AGNT_MONITOR_CHECK_RESULT          (char*)"00011001"

#define VPI_CAN_AGNT_MONITOR_SET_INPUT_DELAY       (char*)"00011010"

/**
 * @subsection Test controller bus agent
 */
#define VPI_TEST_AGNT_TEST_END                     (char*)"00000001"
#define VPI_TEST_AGNT_GET_CFG                      (char*)"00000010"

/**
 * @enum CAN Agent Monitor State.
 * 
 *  CAN_AGENT_MONITOR_DISABLED:
 *      Monitor FIFO can be filled with values to be monitored, sampling
 *      rate and trigger can be configured.
 *  
 *  CAN_AGENT_MONITOR_WAITING_FOR_TRIGGER:
 *      Monitor has been started, but trigger condition has not yet occurred.
 * 
 *  CAN_AGENT_MONITOR_RUNNING:
 *      Monitor is running and monitoring values from Monitor FIFO on "can_tx".
 * 
 *  CAN_AGENT_MONITOR_PASSED:
 *      Monitor has monitored all values from monitor FIFO and it is not running
 *      anymore. During monitoring, no mismatch occurred therefore it PASSED.
 * 
 *  CAN_AGENT_MONITOR_FAILED:
 *      Monitor has monitored all values from monitor FIFO and it is not running
 *      anymore. During monitoring mismatches occured therefore it FAILED.
 */
enum CanAgentMonitorState
{
    CAN_AGENT_MONITOR_DISABLED,
    CAN_AGENT_MONITOR_WAITING_FOR_TRIGGER,
    CAN_AGENT_MONITOR_RUNNING,
    CAN_AGENT_MONITOR_PASSED,
    CAN_AGENT_MONITOR_FAILED
};


/**
 * @enum CAN Agent Monitor Trigger type
 * 
 * CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY:
 *  Trigger immediately after CAN monitor is started.
 * 
 * CAN_AGENT_MONITOR_TRIGGER_RX_RISING:
 *  Trigger on rising edge on can_rx. 
 * 
 * CAN_AGENT_MONITOR_TRIGGER_RX_FALLING:
 *  Trigger on falling edge on can_rx.
 * 
 * CAN_AGENT_MONITOR_TRIGGER_TX_RISING:
 *  Trigger on rising edge on can_tx.
 * 
 * CAN_AGENT_MONITOR_TRIGGER_TX_FALLING:
 *  Trigger on falling edge on can_tx.
 * 
 * CAN_AGENT_MONITOR_TRIGGER_TIME_ELAPSED:
 *  Trigger after certain time has elapsed.
 * 
 * CAN_AGENT_MONITOR_TRIGGER_DRIVER_START:
 *  Trigger when CAN Agent driver starts. This trigger type can be used to start
 *  monitoring at the same time as driver starts driving.
 *
 * CAN_AGENT_MONITOR_TRIGGER_DRIVER_STOP:
 *  Trigger when CAN Agent driver stops.
 */
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

/**
 * @note All below mentioned functions are "blocking" from callers perspective
 *       Therefore they return only once the action they cause is finished
 *       inside simulation.
 */

/******************************************************************************
 * @section Reset agent functions
 * 
 * @defgroup resetAgent Reset Agent
 *****************************************************************************/

/**
 * @ingroup resetAgent
 * 
 * @brief Assert reset.
 */
void resetAgentAssert();

/**
 * @ingroup resetAgent
 * 
 * @brief Deassert reset
 */
void resetAgentDeassert();

/**
 * @ingroup resetAgent
 * 
 * @brief Set reset polarity
 * @param polarity Polarity of reset (Allowed values: 0, 1)
 */
void resetAgentPolaritySet(int polarity);

/**
 * @ingroup resetAgent
 * 
 * @brief Get reset agent polarity
 * @returns Polarity of reset (0, 1)
 */
int resetAgentPolarityGet();


/******************************************************************************
 * @section Clock generator agent functions
 *
 * @defgroup clockGeneratorAgent Clock Generator Agent
 *****************************************************************************/

/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Start clock generator agent.
 * 
 * When clock generator agent is running, it generates clock on its output.
 */
void clockAgentStart();


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Stop clock generator agent.
 * 
 * When clock generator agent is stopped, it does not generate clock on its
 * output.
 */
void clockAgentStop();


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Set clock generator agent period
 * @param clockPeriod Period to be set.
 */
void clockAgentSetPeriod(std::chrono::nanoseconds clockPeriod);


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Get clock generator agent period.
 * @return Clock generator agent period.
 */
std::chrono::nanoseconds clockAgentGetPeriod();


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Set clock generator agent jitter (Jitter of clock period).
 * @param jitter Jitter to be set.
 */
void clockAgentSetJitter(std::chrono::nanoseconds jitter);


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Get clock generator agent jitter (Jitter of clock period).
 * @return Jitter of Clock generator agent.
 */
std::chrono::nanoseconds clockAgentGetJitter();


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Set clock generator agent duty cycle.
 * @param duty Duty cycle to set (valid value between 0 - 100)
 */
void clockAgentSetDuty(int duty);


/**
 * @ingroup clockGeneratorAgent
 * 
 * @brief Get clock generator agent duty cycle.
 * @return Duty cycle of Clock generator agent (between 0 - 100)
 */
int clockAgentGetDuty();


/******************************************************************************
 * @section Memory bus agent functions
 *
 * @defgroup memoryBusAgent Memory Bus Agent
 *****************************************************************************/

/**
 * @ingroup memBusAgent
 * 
 * @brief Start Memory Bus agent.
 */
void memBusAgentStart();


/**
 * @ingroup memBusAgent
 * 
 * @brief Stop Memory Bus agent.
 */
void memBusAgentStop();


/**
 * @ingroup memBusAgent
 * 
 * @brief Execute 32-bit write by Memory bus agent.
 * @param address Address to write into (Must be 4 bytes aligned).
 * @param data Data to be written.
 */
void memBusAgentWrite32(int address, uint32_t data);


/**
 * @ingroup memBusAgent
 * 
 * @brief Execute 16-bit write by Memory bus agent.
 * @param address Address to write into (Must be 2 bytes aligned).
 * @param data Data to be written.
 */
void memBusAgentWrite16(int address, uint16_t data);


/**
 * @ingroup memBusAgent
 * 
 * @brief Execute 8-bit write by Memory bus agent.
 * @param address Address to write into.
 * @param data Data to be written.
 */
void memBusAgentWrite8(int address, uint8_t data);


/**
 * @ingroup memBusAgent
 * 
 * @brief Execute 32-bit read by Memory bus agent.
 * @param address Address to read from (Must be 4 bytes aligned).
 * @return Data read by Memory bus agent.
 */
uint32_t memBusAgentRead32(int address);


/**
 * @ingroup memBusAgent
 * 
 * @brief Execute 16-bit read by Memory bus agent.
 * @param address Address to read from (Must be 2 bytes aligned).
 * @return Data read by Memory bus agent.
 */
uint16_t memBusAgentRead16(int address);


/**
 * @ingroup memBusAgent
 * 
 * @brief Execute 8-bit read by Memory bus agent.
 * @param address Address to read from.
 * @return Data read by Memory bus agent.
 */
uint8_t memBusAgentRead8(int address);


/**
 * @ingroup memBusAgent
 * 
 * @brief Turn on X-mode in Memory Bus agent. In X mode Memory bus agent drives
 *        X on data signals everywhere apart from setup + hold window from
 *        rising edge!
 */
void memBusAgentXModeStart();


/**
 * @ingroup memBusAgent
 * 
 * @brief Turn on X-mode in Memory Bus agent. In X mode Memory bus agent drives
 *        X on data signals everywhere apart from setup + hold window from
 *        rising edge!
 */
void memBusAgentXModeStop();


/**
 * @ingroup memBusAgent
 * 
 * @brief Set X mode setup time.
 * @param setup Setup time.
 * 
 * Setup time must be less than half of Clock of Memory Bus agent!
 */
void memBusAgentSetXModeSetup(std::chrono::nanoseconds setup);


/**
 * @ingroup memBusAgent
 * 
 * @brief Set X mode hold time.
 * @param hold Hold time.
 * 
 * Hold time must be less than half of Clock of Memory Bus agent!
 */
void memBusAgentSetXModeHold(std::chrono::nanoseconds hold);


/**
 * @ingroup memBusAgent
 * 
 * @brief Set Memory Bus agent period.
 * @param period Memory Bus Agent period.
 * 
 * Memory Bus agent period must be set to the same value as period of clock
 * on input clock of Memory Bus agent! This is required to properly calculate
 * setup and hold constraints by Memory bus agent.
 */
void memBusAgentSetPeriod(std::chrono::nanoseconds period);


/**
 * @ingroup memBusAgent
 * 
 * @brief Set Memory Bus agent output delay.
 * @param delay Memory Bus Agent output delay.
 * 
 * Memory bus agent samples data output with output delay from rising edge
 * of clock in case of read access.
 */
void memBusAgentSetOutputDelay(std::chrono::nanoseconds delay);


/******************************************************************************
 * @section CAN Agent functions
 * 
 * @defgroup canAgent CAN Agent
 *****************************************************************************/

/**
 * @ingroup canAgent
 * 
 * @brief Start CAN agent driver.
 * 
 * When driver is enabled, it drives items from Driver FIFO.
 */
void canAgentDriverStart();


/**
 * @ingroup canAgent
 * 
 * @brief Start CAN agent driver.
 * 
 * When driver is disabled, it drives only recessive value.
 */
void canAgentDriverStop();


/**
 * @ingroup canAgent
 * 
 * @brief Flush CAN agent driver FIFO.
 */
void canAgentDriverFlush();


/**
 * @ingroup canAgent
 * 
 * @brief Check if CAN agent driver is driving some item from driver FIFO.
 * @return true when driving is in progress, false otherwise
 */
bool canAgentDriverGetProgress();


/**
 * @ingroup canAgent
 * 
 * @brief Get currently driven value by CAN agent driver.
 * @return Value driven by CAN agent driver.
 */
char canAgentDriverGetDrivenVal();


/**
 * @ingroup canAgent
 * 
 * @brief Insert item to CAN agent driver FIFO.
 * @param drivenValue Logic value corresponding to this item. (This value is
 *                    driven on "can_rx").
 * @param duration Time duration for which this value is driven.
 */
void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration);


/**
 * @ingroup canAgent
 * 
 * @brief Insert item to CAN agent driver FIFO.
 * @param drivenValue Logic value corresponding to this item. (This value is
 *                    driven on "can_rx").
 * @param duration Time duration for which this value is driven.
 * @param msg Message which will be printed in simulator when CAN Agent driver
 *            starts driving this value.
 */
void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg);


/**
 * @ingroup canAgent
 * 
 * @brief Set wait timeout.
 * @param timeout Timeout to be set.
 *
 * Driver Wait timeout is upper threshold for waiting on end of CAN Agent
 * driving from Driver FIFO.
 */
void canAgentDriverSetWaitTimeout(std::chrono::nanoseconds timeout);


/**
 * @ingroup canAgent
 * 
 * @brief Wait till CAN Agent driver ends. Wait time is limited by wait
 *        timeout.
 */
void canAgentDriverWaitFinish();


/**
 * @ingroup canAgent
 * 
 * @brief Insert single item to CAN Agent driver FIFO and wait until this item
 *        is driven. If other items are in CAN Agent driver FIFO, these will be
 *        driven first.
 * @param drivenValue Value to be driven on "can_rx".
 * @param duration Time for which this value will be driven.
 * @param msg Message to be printed in simulator log when driving od this item
 *            starts!
 */
void canAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg);


/**
 * @ingroup canAgent
 * 
 * @brief Insert single item to CAN Agent driver FIFO and wait until this item
 *        is driven. If other items are in CAN Agent driver FIFO, these will be
 *        driven first.
 * @param drivenValue Value to be driven on "can_rx".
 * @param duration Time for which this value will be driven.
 */
void canAgentDriveSingleItem(char vdrivenValuealue, std::chrono::nanoseconds duration);


/**
 * @ingroup canAgent
 * 
 * @brief Drive all items already present in CAN Agent driver FIFO.
 */
void canAgentDriveAllItems();


/**
 * @ingroup canAgent
 * 
 * @brief Start monitor. When monitor is running, it monitors items from CAN
 *        agent monitor FIFO on "can_tx".
 */
void canAgentMonitorStart();


/**
 * @ingroup canAgent
 * 
 * @brief Stop monitor. When monitor is stopped it does not monitor any values
 *        on "can_tx".
 */
void canAgentMonitorStop();


/**
 * @ingroup canAgent
 * 
 * @brief Flush Monitor FIFO.
 */
void canAgentMonitorFlush();


/**
 * @ingroup canAgent
 * 
 * @brief Get Monitor state.
 * @return Current monitor state.
 */
CanAgentMonitorState canAgentMonitorGetState();


/**
 * @ingroup canAgent
 * 
 * @brief Currently monitored value on "can_tx".
 * @return Value of currently monitored item.
 */
char canAgentMonitorGetMonitoredVal();


/**
 * @ingroup canAgent
 * 
 * @brief Insert Item to Monitor FIFO.
 * @param monitorValue Value to be monitored
 * @param duration Time for which monitorValue is monitored.
 * @param sampleRate Sample rate used to check this item during monitoring.
 */
void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration,
                             std::chrono::nanoseconds sampleRate);


/**
 * @ingroup canAgent
 * 
 * @brief Insert Item to Monitor FIFO.
 * @param monitorValue Value to be monitored
 * @param duration Time for which monitorValue is monitored.
 * @param msg Message to be printed when monitoring of this item starts.
 */
void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration,
                             std::chrono::nanoseconds sampleRate, std::string msg);


/**
 * @ingroup canAgent
 * 
 * @brief Configure Monitor Timeout.
 * @param timeout Timeout to be set.
 * 
 * Monitor Wait timeout is upper threshold for waiting on end of CAN Agent
 * monitoring from Monitor FIFO.
 */
void canAgentMonitorSetWaitTimeout(std::chrono::nanoseconds timeout);


/**
 * @ingroup canAgent
 * 
 * @brief Wait till CAN Agent monitor ends.
 * @param timeout
 * 
 * Monitor Wait timeout is upper threshold for waiting on end of CAN Agent
 * monitoring from Monitor FIFO.
 */
void canAgentMonitorWaitFinish();


/**
 * @ingroup canAgent
 * 
 * @brief Wait till CAN Agent monitor ends.
 * @param timeout
 * 
 * Monitor Wait timeout is upper threshold for waiting on end of CAN Agent
 * monitoring from Monitor FIFO.
 */
void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration,
                               std::chrono::nanoseconds sampleRate);


/**
 * @ingroup canAgent
 * 
 * @brief Monitor single Item by CAN Agent.
 * @param monitorValue Value to be monitored on "can_tx"
 * @param duration Time for which to monitor this value.
 * @param msg Message to be printed in simulator log when monitoring of this
 *            value starts.
 * 
 * Monitor Wait timeout is upper threshold for waiting on end of CAN Agent
 * monitoring from Monitor FIFO.
 */
void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration,
                               std::chrono::nanoseconds sampleRate, std::string msg);


/**
 * @ingroup canAgent
 * 
 * @brief Monitor all items from Monitor FIFO.
 */
void canAgentMonitorAllItems();


/**
 * @ingroup canAgent
 * 
 * @brief Set trigger for Monitor.
 * @param trigger Trigger type to be configured.
 */
void canAgentMonitorSetTrigger(CanAgentMonitorTrigger trigger);


/**
 * @ingroup canAgent
 * 
 * @brief Get trigger for Monitor.
 * @return Monitor Trigger type
 */
CanAgentMonitorTrigger canAgentMonitorGetTrigger();


/**
 * @ingroup canAgent
 * 
 * @brief Perform check by a monitor whether result of previous monitoring
 *        was succesfull. Print result to simulator log.
 */
void canAgentCheckResult();


/**
 * @ingroup canAgent
 * 
 * @brief Set Monitor input delay. Monitor will apply additional input delay
 *        after trigger before monitoring first item! This delay corresponds
 *        to DUT input delay.
 * @param inputDelay Input delay to set
 */
void canAgentSetMonitorInputDelay(std::chrono::nanoseconds inputDelay);


/******************************************************************************
 * @section Test controller agent functions
 * 
 * @defgroup testControllerAgent Test controller agent
 *****************************************************************************/

/**
 * @ingroup testControllerAgent
 * 
 * @brief Signal to TB in simulator that test has ended.
 * @param success Test result. This will be passed to VUnit.
 */
void testControllerAgentEndTest(bool success);


/**
 * @ingroup testControllerAgent
 * 
 * @brief 
 * @return
 */
std::chrono::nanoseconds testControllerAgentGetCfgDutClockPeriod();


/**
 * @ingroup testControllerAgent
 * 
 * @brief 
 * @return
 */
int testControllerAgentGetBitTimingElement(std::string elemName);


#endif