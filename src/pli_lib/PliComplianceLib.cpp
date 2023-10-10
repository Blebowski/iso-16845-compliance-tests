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
#include <bitset>
#include <string>
#include <atomic>

#include "PliComplianceLib.hpp"
#include "SimulatorChannel.hpp"


/*****************************************************************************
 * Reset agent functions
 ****************************************************************************/

void ResetAgentAssert()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_RES_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_RST_AGNT_CMD_ASSERT);

    SimulatorChannelProcessRequest();
}


void ResetAgentDeassert()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_RES_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_RST_AGNT_CMD_DEASSERT);

    SimulatorChannelProcessRequest();
}


void ResetAgentPolaritySet(int polarity)
{
    char pol[2];
    sprintf(pol, "%d", polarity);

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_RES_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_RST_AGNT_CMD_POLARITY_SET);
    simulator_channel.pli_data_in = pol;

    SimulatorChannelProcessRequest();
}


int ResetAgentPolarityGet()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_RES_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_RST_AGNT_CMD_POLARITY_GET);

    SimulatorChannelProcessRequest();

    return atoi(&simulator_channel.pli_data_out.at(0));
}


/*****************************************************************************
 * Clock generator agent functions
 ****************************************************************************/

void ClockAgentStart()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_START);

    SimulatorChannelProcessRequest();
}


void ClockAgentStop()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_STOP);

    SimulatorChannelProcessRequest();
}


void ClockAgentSetPeriod(std::chrono::nanoseconds clockPeriod)
{
    unsigned long long timeVal = clockPeriod.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_PERIOD_SET);
    simulator_channel.pli_data_in = std::bitset<PLI_DBUF_SIZE>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


std::chrono::nanoseconds ClockAgentGetPeriod()
{
    unsigned long long readTime;

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_PERIOD_GET);

    SimulatorChannelProcessRequest();
    readTime = std::strtoll(simulator_channel.pli_data_out.c_str(), nullptr, 2) / 1000000;
    return std::chrono::nanoseconds(readTime);
}


void ClockAgentSetJitter(std::chrono::nanoseconds jitter)
{
    unsigned long long timeVal = jitter.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_JITTER_SET);
    simulator_channel.pli_data_in = std::bitset<PLI_DBUF_SIZE>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


std::chrono::nanoseconds ClockAgentGetJitter()
{
    unsigned long long readJitter;

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_JITTER_GET);

    SimulatorChannelProcessRequest();
    readJitter = std::strtoll(simulator_channel.pli_data_out.c_str(), nullptr, 2) / 1000000;
    return std::chrono::nanoseconds(readJitter);
}


void ClockAgentSetDuty(int duty)
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_DUTY_SET);
    simulator_channel.pli_data_in = std::bitset<PLI_DBUF_SIZE>(duty).to_string();

    SimulatorChannelProcessRequest();
}


int ClockAgentGetDuty()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CLK_GEN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CLK_AGNT_CMD_DUTY_GET);

    SimulatorChannelProcessRequest();
    return std::stoi(simulator_channel.pli_data_out.c_str(), nullptr, 2);
}


/*****************************************************************************
 * Memory bus agent functions
 ****************************************************************************/

void MemBusAgentStart()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_START);

    SimulatorChannelProcessRequest();
}


void MemBusAgentStop()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_STOP);

    SimulatorChannelProcessRequest();
}


void MemBusAgentWrite32(int address, uint32_t data)
{
    std::string pli_data_in = "1"; // Use blocking write
    pli_data_in.append("10"); // 32 bit write
    pli_data_in.append(std::bitset<16>(address).to_string());
    pli_data_in.append(std::bitset<32>(data).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_WRITE);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void MemBusAgentWrite16(int address, uint16_t data)
{
    std::string pli_data_in = "1"; // Use blocking write
    pli_data_in.append("01"); // 16 bit write
    pli_data_in.append(std::bitset<16>(address).to_string());
    pli_data_in.append("0000000000000000");
    pli_data_in.append(std::bitset<16>(data).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_WRITE);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void MemBusAgentWrite8(int address, uint8_t data)
{
    std::string pli_data_in = "1"; // Use blocking write
    pli_data_in.append("00"); // 8 bit write
    pli_data_in.append(std::bitset<16>(address).to_string());
    pli_data_in.append("000000000000000000000000");
    pli_data_in.append(std::bitset<8>(data).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_WRITE);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


uint32_t MemBusAgentRead32(int address)
{
    std::string pli_data_in = "";
    pli_data_in.append("10"); // 32 bit access
    pli_data_in.append(std::bitset<16>(address).to_string());
    pli_data_in.append("00000000000000000000000000000000");

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_READ);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();

    return (uint32_t)strtoul(simulator_channel.pli_data_out.c_str(), NULL, 2);
}

uint16_t MemBusAgentRead16(int address)
{
    std::string pli_data_in = "";
    pli_data_in.append("01"); // 16 bit access
    pli_data_in.append(std::bitset<16>(address).to_string());
    pli_data_in.append("00000000000000000000000000000000");

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_READ);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();

    return (uint16_t)strtoul(simulator_channel.pli_data_out.c_str(), NULL, 2);
}


uint8_t MemBusAgentRead8(int address)
{
    std::string pli_data_in = "";
    pli_data_in.append("00"); // 8 bit access
    pli_data_in.append(std::bitset<16>(address).to_string());
    pli_data_in.append("00000000000000000000000000000000");

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_READ);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();

    return (uint8_t)strtoul(simulator_channel.pli_data_out.c_str(), NULL, 2);
}


void MemBusAgentXModeStart()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_X_MODE_START);

    SimulatorChannelProcessRequest();
}


void MemBusAgentXModeStop()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_X_MODE_STOP);

    SimulatorChannelProcessRequest();
}


void memBusAgentSetXModeSetup(std::chrono::nanoseconds setup)
{
    unsigned long long timeVal = setup.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_SET_X_MODE_SETUP);
    simulator_channel.pli_data_in = std::bitset<PLI_DBUF_SIZE>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


void MemBusAgentSetXModeHold(std::chrono::nanoseconds hold)
{
    unsigned long long timeVal = hold.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_SET_X_MODE_HOLD);
    simulator_channel.pli_data_in = std::bitset<PLI_DBUF_SIZE>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


void MemBusAgentSetOutputDelay(std::chrono::nanoseconds delay)
{
    unsigned long long timeVal = delay.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_MEM_BUS_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_MEM_BUS_AGNT_SET_OUTPUT_DELAY);
    simulator_channel.pli_data_in = std::bitset<PLI_DBUF_SIZE>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


void CanAgentDriverStart()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_START);

    SimulatorChannelProcessRequest();
}


void CanAgentDriverStop()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_STOP);

    SimulatorChannelProcessRequest();
}


void CanAgentDriverFlush()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_FLUSH);

    SimulatorChannelProcessRequest();
}


bool CanAgentDriverGetProgress()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_GET_PROGRESS);

    SimulatorChannelProcessRequest();

    if (simulator_channel.pli_data_out.at(0) == '1')
        return true;
    return false;
}


char CanAgentDriverGetDrivenVal()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_GET_DRIVEN_VAL);

    SimulatorChannelProcessRequest();

    return simulator_channel.pli_data_out.c_str()[0];
}


void CanAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    pli_data_in.append(1, drivenValue);    // Driven value
    pli_data_in.append("0");      // No message
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_PUSH_ITEM);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void CanAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    int strLen = msg.length();

    if (strLen > PLI_STR_BUF_SIZE)
        strLen = PLI_STR_BUF_SIZE;

    pli_data_in.append(1, drivenValue); // Driven value
    pli_data_in.append("1");   // Message included
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = true;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_PUSH_ITEM);
    simulator_channel.pli_message_data = msg;
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void CanAgentDriverSetWaitTimeout(std::chrono::nanoseconds timeout)
{
    unsigned long long timeVal = timeout.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_SET_WAIT_TIMEOUT);
    simulator_channel.pli_data_in = std::bitset<64>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


void CanAgentDriverWaitFinish()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_WAIT_FINISH);

    SimulatorChannelProcessRequest();
}


void CanAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    int strLen = msg.length();

    if (strLen > PLI_STR_BUF_SIZE)
        strLen = PLI_STR_BUF_SIZE;

    pli_data_in.append(1, drivenValue); // Driven value
    pli_data_in.append("1");   // Message included
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = true;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM);
    simulator_channel.pli_message_data = msg;
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void CanAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    pli_data_in.append(1, drivenValue);    // Driven value
    pli_data_in.append("0");         // No message
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void CanAgentDriveAllItems()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_DRIVER_DRIVE_ALL_ITEM);

    SimulatorChannelProcessRequest();
}


void CanAgentSetWaitForMonitor(bool waitForMonitor)
{
    std::string pli_data_in = "";
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-1>(0).to_string());
    if (waitForMonitor)
        pli_data_in.append("1");
    else
        pli_data_in.append("0");

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_CMD_SET_WAIT_FOR_MONITOR);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorStart()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_START);

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorStop()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_STOP);

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorFlush()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_FLUSH);

    SimulatorChannelProcessRequest();
}


CanAgentMonitorState CanAgentMonitorGetState()
{
    CanAgentMonitorState retVal;

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_GET_STATE);

    SimulatorChannelProcessRequest();

    if (!simulator_channel.pli_data_out.compare("000"))
        retVal = CanAgentMonitorState::Disabled;
    else if (!simulator_channel.pli_data_out.compare("001"))
        retVal = CanAgentMonitorState::WaitingForTrigger;
    else if (!simulator_channel.pli_data_out.compare("010"))
        retVal = CanAgentMonitorState::Running;
    else if (!simulator_channel.pli_data_out.compare("011"))
        retVal = CanAgentMonitorState::Passed;
    else
        retVal = CanAgentMonitorState::Failed;

    return retVal;
}


char CanAgentMonitorGetMonitoredVal()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_GET_MONITORED_VAL);

    SimulatorChannelProcessRequest();

    return simulator_channel.pli_data_out.at(0);
}

void CanAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration,
                             std::chrono::nanoseconds sampleRate)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    pli_data_in.append(1, monitorValue);    // Driven value
    pli_data_in.append("0");      // No message
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string pli_data_in_2 = "";
    pli_data_in_2.append(std::bitset<PLI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_PUSH_ITEM);
    simulator_channel.pli_data_in = pli_data_in;
    simulator_channel.pli_data_in_2 = pli_data_in_2;

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration,
                             std::chrono::nanoseconds sampleRate, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    int strLen = msg.length();

    if (strLen > PLI_STR_BUF_SIZE)
        strLen = PLI_STR_BUF_SIZE;

    pli_data_in.append(1, monitorValue); // Driven value
    pli_data_in.append("1");   // Message included
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string pli_data_in_2 = "";
    pli_data_in_2.append(std::bitset<PLI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = true;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_PUSH_ITEM);
    simulator_channel.pli_data_in = pli_data_in;
    simulator_channel.pli_message_data = msg;
    simulator_channel.pli_data_in_2 = pli_data_in_2;

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorSetWaitTimeout(std::chrono::nanoseconds timeout)
{
    unsigned long long timeVal = timeout.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_SET_WAIT_TIMEOUT);
    simulator_channel.pli_data_in = std::bitset<64>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorWaitFinish()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_WAIT_FINISH);

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration,
                               std::chrono::nanoseconds sampleRate)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";

    pli_data_in.append(1, monitorValue); // Driven value
    pli_data_in.append("0");   // No Message
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string pli_data_in_2 = "";
    pli_data_in_2.append(std::bitset<PLI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM);
    simulator_channel.pli_data_in = pli_data_in;
    simulator_channel.pli_data_in_2 = pli_data_in_2;

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration,
                               std::chrono::nanoseconds sampleRate, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string pli_data_in = "";
    int strLen = msg.length();

    if (strLen > PLI_STR_BUF_SIZE)
        strLen = PLI_STR_BUF_SIZE;

    pli_data_in.append(1, monitorValue); // Driven value
    pli_data_in.append("1");   // Message included
    pli_data_in.append(std::bitset<PLI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string pli_data_in_2 = "";
    pli_data_in_2.append(std::bitset<PLI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = true;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM);
    simulator_channel.pli_data_in = pli_data_in;
    simulator_channel.pli_data_in_2 = pli_data_in_2;
    simulator_channel.pli_message_data = msg;

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorAllItems()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_MONITOR_ALL_ITEMS);

    SimulatorChannelProcessRequest();
}


void CanAgentMonitorSetTrigger(CanAgentMonitorTrigger trigger)
{
    std::string pli_data_in = "";
    switch (trigger){
    case CanAgentMonitorTrigger::Immediately:
        pli_data_in.append("000");
        break;
    case CanAgentMonitorTrigger::RxRising:
        pli_data_in.append("001");
        break;
    case CanAgentMonitorTrigger::RxFalling:
        pli_data_in.append("010");
        break;
    case CanAgentMonitorTrigger::TxRising:
        pli_data_in.append("011");
        break;
    case CanAgentMonitorTrigger::TxFalling:
        pli_data_in.append("100");
        break;
    case CanAgentMonitorTrigger::TimeElapsed:
        pli_data_in.append("101");
        break;
    case CanAgentMonitorTrigger::DriverStart:
        pli_data_in.append("110");
        break;
    case CanAgentMonitorTrigger::DriverStop:
        pli_data_in.append("111");
        break;
    }

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_SET_TRIGGER);
    simulator_channel.pli_data_in = pli_data_in;

    SimulatorChannelProcessRequest();
}


CanAgentMonitorTrigger CanAgentMonitorGetTrigger()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_GET_TRIGGER);

    SimulatorChannelProcessRequest();

    if (!simulator_channel.pli_data_out.compare("000"))
        return CanAgentMonitorTrigger::Immediately;
    if (!simulator_channel.pli_data_out.compare("001"))
        return CanAgentMonitorTrigger::RxRising;
    if (!simulator_channel.pli_data_out.compare("010"))
        return CanAgentMonitorTrigger::RxFalling;
    if (!simulator_channel.pli_data_out.compare("011"))
        return CanAgentMonitorTrigger::TxRising;
    if (!simulator_channel.pli_data_out.compare("100"))
        return CanAgentMonitorTrigger::TxFalling;
    if (!simulator_channel.pli_data_out.compare("101"))
        return CanAgentMonitorTrigger::TimeElapsed;
    if (!simulator_channel.pli_data_out.compare("110"))
        return CanAgentMonitorTrigger::DriverStart;
    if (!simulator_channel.pli_data_out.compare("111"))
        return CanAgentMonitorTrigger::DriverStop;

    return CanAgentMonitorTrigger::Immediately;
}


void CanAgentCheckResult()
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_CHECK_RESULT);

    SimulatorChannelProcessRequest();
}


void CanAgentSetMonitorInputDelay(std::chrono::nanoseconds inputDelay)
{
    unsigned long long timeVal = inputDelay.count() * 1000000;

    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_MONITOR_SET_INPUT_DELAY);
    simulator_channel.pli_data_in = std::bitset<64>(timeVal).to_string();

    SimulatorChannelProcessRequest();
}


void CanAgentConfigureTxToRxFeedback(bool enable)
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_CAN_AGENT);
    if (enable)
        simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_TX_RX_FEEDBACK_ENABLE);
    else
        simulator_channel.pli_cmd = std::string(PLI_CAN_AGNT_TX_RX_FEEDBACK_DISABLE);

    SimulatorChannelProcessRequest();
}

void TestControllerAgentEndTest(bool success)
{
    simulator_channel.read_access = false;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_TEST_CONTROLLER_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_TEST_AGNT_TEST_END);

    if (success)
        simulator_channel.pli_data_in = std::string("1");
    else
        simulator_channel.pli_data_in = std::string("0");

    SimulatorChannelProcessRequest();
}


std::chrono::nanoseconds TestControllerAgentGetCfgDutClockPeriod()
{
    unsigned long long readTime;

    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = true;
    simulator_channel.pli_dest = std::string(PLI_DEST_TEST_CONTROLLER_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_TEST_AGNT_GET_CFG);
    simulator_channel.pli_message_data = "CFG_DUT_CLOCK_PERIOD";

    SimulatorChannelProcessRequest();

    readTime = std::strtoll(simulator_channel.pli_data_out.c_str(), nullptr, 2) / 1000000;
    return std::chrono::nanoseconds(readTime);
}


int TestControllerAgentGetBitTimingElement(std::string elemName)
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = true;
    simulator_channel.pli_dest = std::string(PLI_DEST_TEST_CONTROLLER_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_TEST_AGNT_GET_CFG);
    simulator_channel.pli_message_data = elemName;

    SimulatorChannelProcessRequest();
    return std::stoi(simulator_channel.pli_data_out.c_str(), nullptr, 2);
}


int TestControllerAgentGetSeed()
{
    simulator_channel.read_access = true;
    simulator_channel.use_msg_data = false;
    simulator_channel.pli_dest = std::string(PLI_DEST_TEST_CONTROLLER_AGENT);
    simulator_channel.pli_cmd = std::string(PLI_TEST_AGNT_GET_SEED);

    SimulatorChannelProcessRequest();
    return std::stoi(simulator_channel.pli_data_out.c_str(), nullptr, 2);
}