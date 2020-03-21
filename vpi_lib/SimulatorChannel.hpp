/**
 * TODO: License
 */

#include <iostream>
#include <atomic>

#ifndef SIMULATOR_CHANNEL
#define SIMULATOR_CHANNEL

/**
 * @enum
 */
enum SimulatorChannelFsm
{
    SIM_CHANNEL_FREE,
    SIM_CHANNEL_REQ_UP,
    SIM_CHANNEL_ACK_UP
};


/**
 * @struct 
 */
struct SimulatorChannel
{
    // Automata for processing the request
    std::atomic<SimulatorChannelFsm> fsm;

    std::string vpiDest;
    std::string vpiCmd;
    std::string vpiDataIn;
    std::string vpiDataOut;
    std::string vpiMessageData;

    // Access type
    std::atomic<bool> readAccess;
    std::atomic<bool> useMsgData;

    // Test context Interface
    std::atomic<bool> req;
};

extern SimulatorChannel simulatorChannel;


/** @brief VPI Callback processing function.
 * 
 * VPI Callback is called periodically by simulator. Therefore this CB is
 * always executed in Simulator context and can alter value on top level VPI
 * signals (without corrupting simulator internals)!
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
 *     to simulator and singals this to SimulatorChannel singleton.
 *  8. Test which issued request processing (in case of blocking processing),
 *     proceeds. If this was a read request, then test can read data from
 *     SimulatorChannel which were returned by simulator on "vpi_data_out" 
 */
extern "C" void processVpiClkCallback();


/**********************************************************************
 * Control functions
 *********************************************************************/

/**
 * @brief
 */
void simulatorChannelStartRequest();

/**
 * @brief
 */
void simulatorChannelWaitRequestDone();

/**
 * @brief
 */
void simulatorChannelProcessRequest();

/**
 * @brief
 */
bool simulatorChannelIsRequestPending();

/**
 * @brief
 */
void simulatorChannelClearRequest();


#endif