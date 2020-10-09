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

#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include <bitset>

#include "SimulatorChannel.hpp"
#include "vpiComplianceLib.hpp"

extern "C" {
    #include "vpi_utils.h"
}


SimulatorChannel simulator_channel
{
    ATOMIC_VAR_INIT(SimulatorChannelFsm::FREE),      // fsm

    "",                                             // vpi_dest
    "",                                             // vpi_cmd
    "",                                             // vpi_data_in
    "",                                             // vpi_data_in_2
    "",                                             // vpi_data_out
    "",                                             // vpi_message_data

    ATOMIC_VAR_INIT(false),                         // read access
    ATOMIC_VAR_INIT(false),                         // use_msg_data

    ATOMIC_VAR_INIT(false),                         // req
};


void SimulatorChannelStartRequest()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    simulator_channel.req.store(true);
}


void SimulatorChannelWaitRequestDone()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    while(simulator_channel.req.load())
        usleep(100);
}


void SimulatorChannelProcessRequest()
{
    SimulatorChannelStartRequest();
    SimulatorChannelWaitRequestDone();
}


bool SimulatorChannelIsRequestPending()
{
    return simulator_channel.req.load();
}


void SimulatorChannelClearRequest()
{
    simulator_channel.req.store(false);
}


void ProcessVpiClkCallback()
{
    std::atomic<bool> req;
    char vpi_read_data[VPI_DBUF_SIZE];
    char vpi_ack[128];

    // Check if there is hanging request on SimulatorChannel!
    std::atomic_thread_fence(std::memory_order_seq_cst);
    req.store(SimulatorChannelIsRequestPending());
    std::atomic_thread_fence(std::memory_order_seq_cst);

    //
    // Callback cannot poll on VPI hanshake since it is blocking for digital
    // simulator! Therefore Callback is processed as automata!
    //
    switch (simulator_channel.fsm.load())
    {
        case SimulatorChannelFsm::FREE:
            if (req.load())
            {
                vpi_drive_str_value(VPI_SIGNAL_DEST, simulator_channel.vpi_dest.c_str());
                vpi_drive_str_value(VPI_SIGNAL_CMD, simulator_channel.vpi_cmd.c_str());
                vpi_drive_str_value(VPI_SIGNAL_DATA_IN, simulator_channel.vpi_data_in.c_str());
                vpi_drive_str_value(VPI_SIGNAL_DATA_IN_2, simulator_channel.vpi_data_in_2.c_str());
                if (simulator_channel.use_msg_data)
                {
                    std::string vector = "";
                    for (size_t i = 0; i < simulator_channel.vpi_message_data.length(); i++)
                        vector.append(std::bitset<8>(simulator_channel.vpi_message_data.at(i)).to_string());

                    vpi_drive_str_value(VPI_STR_BUF_IN, vector.c_str());
                }

                std::atomic_thread_fence(std::memory_order_acquire);
                vpi_drive_str_value(VPI_SIGNAL_REQ, "1");
                simulator_channel.fsm.store(SimulatorChannelFsm::REQ_UP);
                std::atomic_thread_fence(std::memory_order_acquire);
            }
            break;

        case SimulatorChannelFsm::REQ_UP:
            memset(vpi_ack, 0, sizeof(vpi_ack));
            vpi_read_str_value(VPI_SIGNAL_ACK, vpi_ack);
            
            if (strcmp(vpi_ack, "1"))
                return;

            /* Copy back read data for read access */
            if (simulator_channel.read_access)
            {
                vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpi_read_data);
                simulator_channel.vpi_data_out = std::string(vpi_read_data);
            }
            vpi_drive_str_value(VPI_SIGNAL_REQ, "0");
            simulator_channel.fsm.store(SimulatorChannelFsm::ACK_UP);
            std::atomic_thread_fence(std::memory_order_acquire);
            break;

        case SimulatorChannelFsm::ACK_UP:
            memset(vpi_ack, 0, sizeof(vpi_ack));

            vpi_read_str_value(VPI_SIGNAL_ACK, vpi_ack);
            if (strcmp(vpi_ack, (char*) "0"))
                return;
            vpi_drive_str_value(VPI_SIGNAL_REQ, "0");
            simulator_channel.fsm.store(SimulatorChannelFsm::FREE);
            std::atomic_thread_fence(std::memory_order_acquire);
            SimulatorChannelClearRequest();
            std::atomic_thread_fence(std::memory_order_acquire);
            break;

        default:
            break;
    }
}