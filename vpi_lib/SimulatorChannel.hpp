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

#include <iostream>
#include <atomic>

#ifndef SIMULATOR_CHANNEL
#define SIMULATOR_CHANNEL

/**
 * @enum State machine for processing of request to simulator.
 */
enum SimulatorChannelFsm
{
    SIM_CHANNEL_FREE,
    SIM_CHANNEL_REQ_UP,
    SIM_CHANNEL_ACK_UP
};


/**
 * @struct Shared memory channel for issuing request to simulator.
 */
struct SimulatorChannel
{
    /* 
     * FSM for request processing.
     * 
     * THIS SHOULD NOT BE DIRECTLY ACCESSES.
     * 
     * Only simulator reads/modifies it as it processes requests!
     */
    std::atomic<SimulatorChannelFsm> fsm;

    /**
     * VPI Destination.
     * Indicates agent in TB to which request will be sent. This will be
     * translated to "vpi_dest" signal in TB.
     */
    std::string vpi_dest;

    /**
     * VPI Command
     * Indicates command which will be sent to an agent given by "vpi_dest".
     * This will be translated to "vpi_cmd" signal in TB.
     */
    std::string vpi_cmd;

    /**
     * VPI Data In
     * Input data for request to simulator. Meaning of these data is command
     * specific (vpi_cmd) for each command. This will be translated to
     * "vpi_data_in" signal in TB.
     */
    std::string vpi_data_in;

    /**
     * VPI Data In 2
     * Input data for request to simulator. Additional data buffer. Meaning is
     * command specific (vpi_cmd) for each command. This will be translated to
     * "vpi_data_in_2" signal in TB.
     */
    std::string vpi_data_in_2;

    /**
     * VPI Data Out
     * Output data from simulator for a request. Meaning of these data is command
     * specific (vpi_cmd) for each command. This value is taken from "vpi_data_out"
     * signal in TB! Data are obtained only when "read_access = true".
     */
    std::string vpi_data_out;

    /**
     * VPI Message data
     * Input data which can send additional information (like print message in
     * case of driver/monitor) as part of request to simulator. These data are
     * interpreted only when "use_msg_data = true". These data are driven on
     * "vpi_str_buf_in" signal in TB.
     */
    std::string vpi_message_data;

    /**
     * Read access
     * Indicates vpi_data_out signal shall be sampled as part of this request and
     * data shall be returned in "vpi_data_out"
     */
    std::atomic<bool> read_access;

    /**
     * Use message data
     * Indicates "vpi_str_buf_in" shall be driven by "vpi_message_data". This can
     * be used to provide additional information (like debug message) to TB!
     */
    std::atomic<bool> use_msg_data;

    /**
     * A request variable.
     * 
     * THIS SHOULD NOT BE DIRECTLY ACCESSES.
     * 
     * Only simulator reads/modifies it as it processes requests.
     */
    std::atomic<bool> req;
};

extern SimulatorChannel simulator_channel;


/** @brief VPI Callback processing function.
 * 
 * VPI Callback is called periodically by simulator. Therefore this CB is
 * always executed in Simulator context and can alter value on top level VPI
 * signals (without corrupting simulator internals)!
 * 
 * VPI Callback alternates FSM of Simulator Channel.
 * 
 * The operation of requests from test to Simulator is following:
 *  1. Test context configures VPI command, VPI Destination and VPI Data and
 *     issues a request processing. This can be blocking (processSimulatorRequest)
 *     or non-blocking (startSimulatorRequest).
 *  2. VPI callback is called in simulator context and it detects pending request.
 *     VPI callback drives "vpi_data_in", "vpi_cmd", "vpi_dest" and issues "vpi_req".
 *  3. Simulator proceeds with simulation and notices "vpi_req". It processes it
 *     and delivers it to dedicated agent in TB.
 *  4. Simulator issues ACK on "vpi_ack"
 *  5. VPI callback is called in simulator context and it detects that "vpi_ack"
 *     is equal to "1". If this is a read access, "vpi_data_out" are read back to
 *     SimulatorChannel. VPI callback drives "vpi_req" back to 0.
 *  6. Simulator proceeds and it notices that "vpi_req" is 0. It drives "vpi_ack"
 *     to 0.
 *  7. VPI callback is called in simulator context and it detects that "vpi_ack"
 *     is equal to "0". This finishes processing of this handshake-like request
 *     to simulator and signals this to SimulatorChannel singleton.
 *  8. Test which issued request processing (in case of blocking processing),
 *     proceeds (SimulatorChannelProcessRequest returns). If this was a read
 *     request, then test can read data from SimulatorChannel which were returned
 *     by simulator on "vpi_data_out".
 */
extern "C" void ProcessVpiClkCallback();


/**********************************************************************
 * Control functions
 *********************************************************************/

/**
 * @brief Issue request to simulator via Simulator Channel.
 * 
 * Once all "vpi_" prefixed attributes are filled, this function issues request
 * to simulator.
 * 
 * This function is non-blocking.
 * 
 * Do not call this function multiple times without waiting for the end of
 * previous request!
 */
void SimulatorChannelStartRequest();


/**
 * @brief Wait till request in Simulator Channel is processed.
 */
void SimulatorChannelWaitRequestDone();


/**
 * @brief Issue request to simulator via Simulator Channel.
 * 
 * Once all "vpi_" prefixed attributes are filled, this function issues request
 * to simulator.
 * 
 * This function is blocking, it returns only after the request was processed!
 */
void SimulatorChannelProcessRequest();


/**
 * @brief Indicates there was a request issued on a Simulator channel.
 */
bool SimulatorChannelIsRequestPending();


/**
 * @brief Clear hanging request on a Simulator Channel.
 */
void SimulatorChannelClearRequest();


#endif