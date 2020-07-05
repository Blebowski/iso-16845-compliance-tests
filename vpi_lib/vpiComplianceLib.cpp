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
#include <bitset>
#include <string>
#include <atomic>

#include "vpiComplianceLib.hpp"
#include "SimulatorChannel.hpp"


/*****************************************************************************
 * Reset agent functions
 ****************************************************************************/

void resetAgentAssert()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_RES_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_RST_AGNT_CMD_ASSERT);

    simulatorChannelProcessRequest();
}


void resetAgentDeassert()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_RES_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_RST_AGNT_CMD_DEASSERT);
    
    simulatorChannelProcessRequest();
}


void resetAgentPolaritySet(int polarity)
{
    char pol[2];
    sprintf(pol, "%d", polarity);

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_RES_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_RST_AGNT_CMD_POLARITY_SET);
    simulatorChannel.vpiDataIn = pol;
    
    simulatorChannelProcessRequest();
}


int resetAgentPolarityGet()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_RES_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_RST_AGNT_CMD_POLARITY_GET);
    
    simulatorChannelProcessRequest();

    return atoi(&simulatorChannel.vpiDataOut.at(0));
}


/*****************************************************************************
 * Clock generator agent functions
 ****************************************************************************/

void clockAgentStart()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_START);
    
    simulatorChannelProcessRequest();
}


void clockAgentStop()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_STOP);
    
    simulatorChannelProcessRequest();
}


void clockAgentSetPeriod(std::chrono::nanoseconds clockPeriod)
{
    unsigned long long timeVal = clockPeriod.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_PERIOD_SET);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


std::chrono::nanoseconds clockAgentGetPeriod()
{
    unsigned long long readTime;

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_PERIOD_GET);
    
    simulatorChannelProcessRequest();
    readTime = std::strtoll(simulatorChannel.vpiDataOut.c_str(), nullptr, 2) / 1000000;
    return std::chrono::nanoseconds(readTime);
}


void clockAgentSetJitter(std::chrono::nanoseconds jitter)
{
    unsigned long long timeVal = jitter.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_JITTER_SET);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


std::chrono::nanoseconds clockAgentGetJitter()
{
    unsigned long long readJitter;

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_JITTER_GET);
    
    simulatorChannelProcessRequest();
    readJitter = std::strtoll(simulatorChannel.vpiDataOut.c_str(), nullptr, 2) / 1000000;
    return std::chrono::nanoseconds(readJitter);
}


void clockAgentSetDuty(int duty)
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_DUTY_SET);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(duty).to_string();
    
    simulatorChannelProcessRequest();
}


int clockAgentGetDuty()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CLK_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CLK_AGNT_CMD_DUTY_GET);
    
    simulatorChannelProcessRequest();
    return std::stoi(simulatorChannel.vpiDataOut.c_str(), nullptr, 2);
}


/*****************************************************************************
 * Memory bus agent functions
 ****************************************************************************/

void memBusAgentStart()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_START);
    
    simulatorChannelProcessRequest();
}


void memBusAgentStop()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_STOP);
    
    simulatorChannelProcessRequest();
}


void memBusAgentWrite32(int address, uint32_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("10"); // 32 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append(std::bitset<32>(data).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_WRITE);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


void memBusAgentWrite16(int address, uint16_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("01"); // 16 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("0000000000000000");
    vpiDataIn.append(std::bitset<16>(data).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_WRITE);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


void memBusAgentWrite8(int address, uint8_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("00"); // 8 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("000000000000000000000000");
    vpiDataIn.append(std::bitset<8>(data).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_WRITE);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


uint32_t memBusAgentRead32(int address)
{
    std::string vpiDataIn = "";
    vpiDataIn.append("10"); // 32 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_READ);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();

    return (uint32_t)strtoul(simulatorChannel.vpiDataOut.c_str(), NULL, 2);
}

uint16_t memBusAgentRead16(int address)
{
    std::string vpiDataIn = "";
    vpiDataIn.append("01"); // 16 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_READ);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();

    return (uint16_t)strtoul(simulatorChannel.vpiDataOut.c_str(), NULL, 2);
}


uint8_t memBusAgentRead8(int address)
{
    std::string vpiDataIn = "";
    vpiDataIn.append("00"); // 8 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_READ);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();

    return (uint8_t)strtoul(simulatorChannel.vpiDataOut.c_str(), NULL, 2);
}


void memBusAgentXModeStart()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_X_MODE_START);
    
    simulatorChannelProcessRequest();
}


void memBusAgentXModeStop()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_X_MODE_STOP);
    
    simulatorChannelProcessRequest();
}


void memBusAgentSetXModeSetup(std::chrono::nanoseconds setup)
{
    unsigned long long timeVal = setup.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_SET_X_MODE_SETUP);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


void memBusAgentSetXModeHold(std::chrono::nanoseconds hold)
{
    unsigned long long timeVal = hold.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_SET_X_MODE_HOLD);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


void memBusAgentSetPeriod(std::chrono::nanoseconds period)
{
    unsigned long long timeVal = period.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_SET_PERIOD);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


void memBusAgentSetOutputDelay(std::chrono::nanoseconds delay)
{
    unsigned long long timeVal = delay.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_MEM_BUS_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_MEM_BUS_AGNT_SET_OUTPUT_DELAY);
    simulatorChannel.vpiDataIn = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


void canAgentDriverStart()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_START);
    
    simulatorChannelProcessRequest();
}


void canAgentDriverStop()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_STOP);
    
    simulatorChannelProcessRequest();
}


void canAgentDriverFlush()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_FLUSH);
    
    simulatorChannelProcessRequest();
}


bool canAgentDriverGetProgress()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_GET_PROGRESS);
    
    simulatorChannelProcessRequest();

    if (simulatorChannel.vpiDataOut.at(0) == '1')
        return true;
    return false;
}


char canAgentDriverGetDrivenVal()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_GET_DRIVEN_VAL);
    
    simulatorChannelProcessRequest();

    return simulatorChannel.vpiDataOut.c_str()[0];
}


void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    vpiDataIn.append(1, drivenValue);    // Driven value
    vpiDataIn.append("0");      // No message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_PUSH_ITEM);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    vpiDataIn.append(1, drivenValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = true;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_PUSH_ITEM);
    simulatorChannel.vpiMessageData = msg;
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


void canAgentDriverSetWaitTimeout(std::chrono::nanoseconds timeout)
{
    unsigned long long timeVal = timeout.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_SET_WAIT_TIMEOUT);
    simulatorChannel.vpiDataIn = std::bitset<64>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


void canAgentDriverWaitFinish()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_WAIT_FINISH);
    
    simulatorChannelProcessRequest();
}


void canAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    vpiDataIn.append(1, drivenValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = true;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM);
    simulatorChannel.vpiMessageData = msg;
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


void canAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    vpiDataIn.append(1, drivenValue);    // Driven value
    vpiDataIn.append("0");         // No message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


void canAgentDriveAllItems()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_DRIVER_DRIVE_ALL_ITEM);
    
    simulatorChannelProcessRequest();
}


void canAgentSetWaitForMonitor(bool waitForMonitor)
{
    std::string vpiDataIn = "";
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-1>(0).to_string());
    if (waitForMonitor)
        vpiDataIn.append("1");
    else
        vpiDataIn.append("0");

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_CMD_SET_WAIT_FOR_MONITOR);
    simulatorChannel.vpiDataIn = vpiDataIn;

    simulatorChannelProcessRequest();
}


void canAgentMonitorStart()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_START);
    
    simulatorChannelProcessRequest();
}


void canAgentMonitorStop()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_STOP);
    
    simulatorChannelProcessRequest();
}


void canAgentMonitorFlush()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_FLUSH);
    
    simulatorChannelProcessRequest();
}


CanAgentMonitorState canAgentMonitorGetState()
{
    CanAgentMonitorState retVal;

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_GET_STATE);
    
    simulatorChannelProcessRequest();

    if (!simulatorChannel.vpiDataOut.compare("000"))
        retVal = CAN_AGENT_MONITOR_DISABLED;
    else if (!simulatorChannel.vpiDataOut.compare("001"))
        retVal = CAN_AGENT_MONITOR_WAITING_FOR_TRIGGER;
    else if (!simulatorChannel.vpiDataOut.compare("010"))
        retVal = CAN_AGENT_MONITOR_RUNNING;
    else if (!simulatorChannel.vpiDataOut.compare("011"))
        retVal = CAN_AGENT_MONITOR_PASSED;
    else
        retVal = CAN_AGENT_MONITOR_FAILED;

    return retVal;
}


char canAgentMonitorGetMonitoredVal()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_GET_MONITORED_VAL);
    
    simulatorChannelProcessRequest();

    return simulatorChannel.vpiDataOut.at(0);
}

void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration,
                             std::chrono::nanoseconds sampleRate)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    vpiDataIn.append(1, monitorValue);    // Driven value
    vpiDataIn.append("0");      // No message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string vpiDataIn2 = "";
    vpiDataIn2.append(std::bitset<VPI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_PUSH_ITEM);
    simulatorChannel.vpiDataIn = vpiDataIn;
    simulatorChannel.vpiDataIn2 = vpiDataIn2;

    simulatorChannelProcessRequest();
}


void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration,
                             std::chrono::nanoseconds sampleRate, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    vpiDataIn.append(1, monitorValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string vpiDataIn2 = "";
    vpiDataIn2.append(std::bitset<VPI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = true;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_PUSH_ITEM);
    simulatorChannel.vpiDataIn = vpiDataIn;
    simulatorChannel.vpiMessageData = msg;
    simulatorChannel.vpiDataIn2 = vpiDataIn2;

    simulatorChannelProcessRequest();
}


void canAgentMonitorSetWaitTimeout(std::chrono::nanoseconds timeout)
{
    unsigned long long timeVal = timeout.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_SET_WAIT_TIMEOUT);
    simulatorChannel.vpiDataIn = std::bitset<64>(timeVal).to_string();
    
    simulatorChannelProcessRequest();
}


void canAgentMonitorWaitFinish()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_WAIT_FINISH);
    
    simulatorChannelProcessRequest();
}


void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration,
                               std::chrono::nanoseconds sampleRate)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";

    vpiDataIn.append(1, monitorValue); // Driven value
    vpiDataIn.append("0");   // No Message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string vpiDataIn2 = "";
    vpiDataIn2.append(std::bitset<VPI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM);
    simulatorChannel.vpiDataIn = vpiDataIn;
    simulatorChannel.vpiDataIn2 = vpiDataIn2;

    simulatorChannelProcessRequest();
}


void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration,
                               std::chrono::nanoseconds sampleRate, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    vpiDataIn.append(1, monitorValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    unsigned long long sampleRateVal = sampleRate.count() * 1000000;
    std::string vpiDataIn2 = "";
    vpiDataIn2.append(std::bitset<VPI_DBUF_SIZE-2>(sampleRateVal).to_string());

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = true;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM);
    simulatorChannel.vpiDataIn = vpiDataIn;
    simulatorChannel.vpiDataIn2 = vpiDataIn2;
    simulatorChannel.vpiMessageData = msg;

    simulatorChannelProcessRequest();
}


void canAgentMonitorAllItems()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_MONITOR_ALL_ITEMS);
    
    simulatorChannelProcessRequest();
}


void canAgentMonitorSetTrigger(CanAgentMonitorTrigger trigger)
{
    std::string vpiDataIn = "";
    switch (trigger){
    case CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY:
        vpiDataIn.append("000");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_RX_RISING:
        vpiDataIn.append("001");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_RX_FALLING:
        vpiDataIn.append("010");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_TX_RISING:
        vpiDataIn.append("011");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_TX_FALLING:
        vpiDataIn.append("100");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_TIME_ELAPSED:
        vpiDataIn.append("101");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_DRIVER_START:
        vpiDataIn.append("110");
        break;
    case CAN_AGENT_MONITOR_TRIGGER_DRIVER_STOP:
        vpiDataIn.append("111");
        break;
    }

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_SET_TRIGGER);
    simulatorChannel.vpiDataIn = vpiDataIn;
    
    simulatorChannelProcessRequest();
}


CanAgentMonitorTrigger canAgentMonitorGetTrigger()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_GET_TRIGGER);
    
    simulatorChannelProcessRequest();

    if (!simulatorChannel.vpiDataOut.compare("000"))
        return CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY;
    if (!simulatorChannel.vpiDataOut.compare("001"))
        return CAN_AGENT_MONITOR_TRIGGER_RX_RISING;
    if (!simulatorChannel.vpiDataOut.compare("010"))
        return CAN_AGENT_MONITOR_TRIGGER_RX_FALLING;
    if (!simulatorChannel.vpiDataOut.compare("011"))
        return CAN_AGENT_MONITOR_TRIGGER_TX_RISING;
    if (!simulatorChannel.vpiDataOut.compare("100"))
        return CAN_AGENT_MONITOR_TRIGGER_TX_FALLING;
    if (!simulatorChannel.vpiDataOut.compare("101"))
        return CAN_AGENT_MONITOR_TRIGGER_TIME_ELAPSED;
    if (!simulatorChannel.vpiDataOut.compare("110"))
        return CAN_AGENT_MONITOR_TRIGGER_DRIVER_START;
    if (!simulatorChannel.vpiDataOut.compare("111"))
        return CAN_AGENT_MONITOR_TRIGGER_DRIVER_STOP;

    return CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY;
}


void canAgentCheckResult()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_CHECK_RESULT);
    
    simulatorChannelProcessRequest();
}


void canAgentSetMonitorInputDelay(std::chrono::nanoseconds inputDelay)
{
    unsigned long long timeVal = inputDelay.count() * 1000000;

    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_MONITOR_SET_INPUT_DELAY);
    simulatorChannel.vpiDataIn = std::bitset<64>(timeVal).to_string();

    simulatorChannelProcessRequest();
}


void canAgentConfigureTxToRxFeedback(bool enable)
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_CAN_AGENT);
    if (enable)
        simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_TX_RX_FEEDBACK_ENABLE);
    else
        simulatorChannel.vpiCmd = std::string(VPI_CAN_AGNT_TX_RX_FEEDBACK_DISABLE);

    simulatorChannelProcessRequest();
}

void testControllerAgentEndTest(bool success)
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_TEST_CONTROLLER_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_TEST_AGNT_TEST_END);
    
    if (success)
        simulatorChannel.vpiDataIn = std::string("1");
    else
        simulatorChannel.vpiDataIn = std::string("0");

    simulatorChannelProcessRequest();
}


std::chrono::nanoseconds testControllerAgentGetCfgDutClockPeriod()
{
    unsigned long long readTime;

    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = true;
    simulatorChannel.vpiDest = std::string(VPI_DEST_TEST_CONTROLLER_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_TEST_AGNT_GET_CFG);
    simulatorChannel.vpiMessageData = "CFG_DUT_CLOCK_PERIOD";

    simulatorChannelProcessRequest();

    readTime = std::strtoll(simulatorChannel.vpiDataOut.c_str(), nullptr, 2) / 1000000;
    return std::chrono::nanoseconds(readTime);
}


int testControllerAgentGetBitTimingElement(std::string elemName)
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = true;
    simulatorChannel.vpiDest = std::string(VPI_DEST_TEST_CONTROLLER_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_TEST_AGNT_GET_CFG);
    simulatorChannel.vpiMessageData = elemName;

    simulatorChannelProcessRequest();
    return std::stoi(simulatorChannel.vpiDataOut.c_str(), nullptr, 2);
}


int testControllerAgentGetSeed()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_TEST_CONTROLLER_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_TEST_AGNT_GET_SEED);

    simulatorChannelProcessRequest();
    return std::stoi(simulatorChannel.vpiDataOut.c_str(), nullptr, 2);
}