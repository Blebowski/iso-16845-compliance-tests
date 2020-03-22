/**
 * TODO:
 */

#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#include "SimulatorChannel.hpp"
#include "vpi_compliance_lib.hpp"

extern "C" {
    #include "vpi_utils.h"
}


SimulatorChannel simulatorChannel =
{
    .fsm = ATOMIC_VAR_INIT(SIM_CHANNEL_FREE),

    .vpiDest = "",
    .vpiCmd = "",
    .vpiDataIn = "",
    .vpiDataOut = "",
    .vpiMessageData = "",

    .readAccess = ATOMIC_VAR_INIT(false),
    .useMsgData = ATOMIC_VAR_INIT(false),

    .req = ATOMIC_VAR_INIT(false),
};


void simulatorChannelStartRequest()
{
    simulatorChannel.req.store(true);
}


void simulatorChannelWaitRequestDone()
{
    while(simulatorChannel.req.load())
        usleep(100);
}


void simulatorChannelProcessRequest()
{
    simulatorChannelStartRequest();
    simulatorChannelWaitRequestDone();
}


bool simulatorChannelIsRequestPending()
{
    return simulatorChannel.req.load();
}


void simulatorChannelClearRequest()
{
    simulatorChannel.req.store(false);
}


void processVpiClkCallback()
{
    std::atomic<bool> req;
    char vpiReadData[VPI_DBUF_SIZE];
    char vpiAck[128];

    // Check if there is hanging request on SimulatorChannel!
    std::atomic_thread_fence(std::memory_order_seq_cst);
    req.store(simulatorChannelIsRequestPending());
    std::atomic_thread_fence(std::memory_order_seq_cst);

    //
    // Callback cannot poll on VPI hanshake since it is blocking for digital
    // simulator! Therefore Callback is processed as automata!
    //
    switch (simulatorChannel.fsm.load())
    {
        case SIM_CHANNEL_FREE:
            if (req.load())
            {
                vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)simulatorChannel.vpiDest.c_str());
                vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)simulatorChannel.vpiCmd.c_str());
                vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)simulatorChannel.vpiDataIn.c_str());
                if (simulatorChannel.useMsgData)
                    vpi_drive_str_value(VPI_STR_BUF_IN, (char*)simulatorChannel.vpiMessageData.c_str());

                std::atomic_thread_fence(std::memory_order_acquire);
                vpi_drive_str_value(VPI_SIGNAL_REQ, (char*) "1");
                simulatorChannel.fsm.store(SIM_CHANNEL_REQ_UP);
                std::atomic_thread_fence(std::memory_order_acquire);
            }
            break;

        case SIM_CHANNEL_REQ_UP:
            memset(vpiAck, 0, sizeof(vpiAck));
            vpi_read_str_value(VPI_SIGNAL_ACK, vpiAck);
            
            if (strcmp(vpiAck, "1"))
                return;
            
            // Copy back read data for read access
            if (simulatorChannel.readAccess)
            {
                vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiReadData);
                simulatorChannel.vpiDataOut = std::string(vpiReadData);
            }
            vpi_drive_str_value(VPI_SIGNAL_REQ, (char*) "0");
            simulatorChannel.fsm.store(SIM_CHANNEL_ACK_UP);
            std::atomic_thread_fence(std::memory_order_acquire);
            break;

        case SIM_CHANNEL_ACK_UP:
            memset(vpiAck, 0, sizeof(vpiAck));

            vpi_read_str_value(VPI_SIGNAL_ACK, vpiAck);
            if (strcmp(vpiAck, (char*) "0"))
                return;
            vpi_drive_str_value(VPI_SIGNAL_REQ, (char*) "0");
            simulatorChannel.fsm.store(SIM_CHANNEL_FREE);
            std::atomic_thread_fence(std::memory_order_acquire);
            simulatorChannelClearRequest();
            std::atomic_thread_fence(std::memory_order_acquire);
            break;

        default:
            break;
        }
}