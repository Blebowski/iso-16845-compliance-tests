/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
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
#include "PliComplianceLib.hpp"

extern "C" {
    #include "pli_utils.h"
}


SimulatorChannel simulator_channel
{
    ATOMIC_VAR_INIT(SimulatorChannelFsm::FREE),     // fsm

    "",                                             // pli_dest
    "",                                             // pli_cmd
    "",                                             // pli_data_in
    "",                                             // pli_data_in_2
    "",                                             // pli_data_out
    "",                                             // pli_message_data

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
    char pli_read_data[2 * PLI_DBUF_SIZE];
    char pli_ack[128];

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
                pli_drive_str_value(PLI_SIGNAL_DEST, simulator_channel.pli_dest.c_str());
                pli_drive_str_value(PLI_SIGNAL_CMD, simulator_channel.pli_cmd.c_str());
                pli_drive_str_value(PLI_SIGNAL_DATA_IN, simulator_channel.pli_data_in.c_str());
                pli_drive_str_value(PLI_SIGNAL_DATA_IN_2, simulator_channel.pli_data_in_2.c_str());
                if (simulator_channel.use_msg_data)
                {
                    std::string vector = "";
                    for (size_t i = 0; i < simulator_channel.pli_message_data.length(); i++)
                        vector.append(std::bitset<8>(simulator_channel.pli_message_data.at(i)).to_string());

                    pli_drive_str_value(PLI_STR_BUF_IN, vector.c_str());
                }

                std::atomic_thread_fence(std::memory_order_acquire);
                pli_drive_str_value(PLI_SIGNAL_REQ, "1");
                simulator_channel.fsm.store(SimulatorChannelFsm::REQ_UP);
                std::atomic_thread_fence(std::memory_order_acquire);
            }
            break;

        case SimulatorChannelFsm::REQ_UP:
            memset(pli_ack, 0, sizeof(pli_ack));
            pli_read_str_value(PLI_SIGNAL_ACK, pli_ack);

            if (strcmp(pli_ack, "1"))
                return;

            /* Copy back read data for read access */
            if (simulator_channel.read_access)
            {
                pli_read_str_value(PLI_SIGNAL_DATA_OUT, pli_read_data);
                simulator_channel.pli_data_out = std::string(pli_read_data);
            }
            pli_drive_str_value(PLI_SIGNAL_REQ, "0");
            simulator_channel.fsm.store(SimulatorChannelFsm::ACK_UP);
            std::atomic_thread_fence(std::memory_order_acquire);
            break;

        case SimulatorChannelFsm::ACK_UP:
            memset(pli_ack, 0, sizeof(pli_ack));

            pli_read_str_value(PLI_SIGNAL_ACK, pli_ack);
            if (strcmp(pli_ack, (char*) "0"))
                return;
            pli_drive_str_value(PLI_SIGNAL_REQ, "0");
            simulator_channel.fsm.store(SimulatorChannelFsm::FREE);
            std::atomic_thread_fence(std::memory_order_acquire);
            SimulatorChannelClearRequest();
            std::atomic_thread_fence(std::memory_order_acquire);
            break;

        default:
            break;
    }
}