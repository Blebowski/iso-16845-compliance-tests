/**
 * TODO:License
 */

#include <unistd.h>
#include <stdlib.h>
#include <bitset>
#include <string>
#include <atomic>


extern "C" {
    #include "vpi_utils.h"
}

#include "vpi_compliance_lib.hpp"
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
    std::atomic_thread_fence(std::memory_order_seq_cst);

    simulatorChannelProcessRequest();
}


void resetAgentDeassert()
{
    simulatorChannel.readAccess = false;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_RES_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_RST_AGNT_CMD_DEASSERT);
    std::atomic_thread_fence(std::memory_order_seq_cst);

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
    std::atomic_thread_fence(std::memory_order_seq_cst);

    simulatorChannelProcessRequest();
}


int resetAgentPolarityGet()
{
    simulatorChannel.readAccess = true;
    simulatorChannel.useMsgData = false;
    simulatorChannel.vpiDest = std::string(VPI_DEST_RES_GEN_AGENT);
    simulatorChannel.vpiCmd = std::string(VPI_RST_AGNT_CMD_POLARITY_GET);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    simulatorChannelProcessRequest();

    return atoi((char*)simulatorChannel.vpiDataOut[0]);
}


/*****************************************************************************
 * Clock generator agent functions
 ****************************************************************************/

int clockAgentStart()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_START);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


int clockAgentStop()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_STOP);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


int clockAgentSetPeriod(std::chrono::nanoseconds clockPeriod)
{
    unsigned long long timeVal = clockPeriod.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char *)binStr.c_str());
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_PERIOD_SET);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


std::chrono::nanoseconds clockAgentGetPeriod()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpiDataOut, 0, sizeof(vpiDataOut));
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_PERIOD_GET);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    readTime = std::strtoll(vpiDataOut, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


int clockAgentSetJitter(std::chrono::nanoseconds clockPeriod)
{
    unsigned long long timeVal = clockPeriod.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_JITTER_SET);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


std::chrono::nanoseconds clockAgentGetJitter()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpiDataOut, 0, sizeof(vpiDataOut));
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_JITTER_GET);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    readTime = std::strtoll(vpiDataOut, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


int clockAgentSetDuty(int duty)
{
    std::string binValStr = std::bitset<VPI_DBUF_SIZE>(duty).to_string();
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_DUTY_SET);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)binValStr.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


int clockAgentGetDuty()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    int duty;

    memset(vpiDataOut, 0, sizeof(vpiDataOut));
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_DUTY_GET);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    duty = std::stoi(vpiDataOut, nullptr, 2);
    vpi_end_handshake();

    return duty;
}


/*****************************************************************************
 * Memory bus agent functions
 ****************************************************************************/

void memBusAgentStart()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_START);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentStop()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_STOP);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentWrite32(int address, uint32_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("10"); // 32 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append(std::bitset<32>(data).to_string());
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_WRITE);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentWrite16(int address, uint16_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("01"); // 16 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("0000000000000000");
    vpiDataIn.append(std::bitset<16>(data).to_string());
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_WRITE);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentWrite8(int address, uint8_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("00"); // 8 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("000000000000000000000000");
    vpiDataIn.append(std::bitset<8>(data).to_string());
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_WRITE);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


uint32_t memBusAgentRead32(int address)
{
    std::string vpiDataIn = "";
    char vpiDataOut[VPI_DBUF_SIZE];
    char *endPtr = NULL;
    uint32_t readData;

    vpiDataIn.append("10"); // 32 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_READ);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    printf("VPI DATA BUFFER READ: %s\n\n", vpiDataOut);

    readData = (uint32_t)strtoul(vpiDataOut, &endPtr, 2);
    vpi_end_handshake();

    return readData;
}

uint16_t memBusAgentRead16(int address)
{
    std::string vpiDataIn = "";
    char vpiDataOut[VPI_DBUF_SIZE];
    uint16_t readData;

    vpiDataIn.append("01"); // 16 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_READ);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    readData = (uint16_t)strtoul(vpiDataOut, NULL, 2);
    vpi_end_handshake();

    return readData;
}


uint8_t memBusAgentRead8(int address)
{
    std::string vpiDataIn = "";
    char vpiDataOut[VPI_DBUF_SIZE];
    memset(vpiDataOut, 0, sizeof(vpiDataOut));
    uint8_t readData;

    vpiDataIn.append("00"); // 8 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_READ);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    readData = (uint8_t)strtoul(vpiDataOut, NULL, 2);
    vpi_end_handshake();

    return readData;
}


void memBusAgentXModeStart()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_X_MODE_START);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentXModeStop()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_X_MODE_STOP);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentSetXModeSetup(std::chrono::nanoseconds setup)
{
    unsigned long long timeVal = setup.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_X_MODE_SETUP);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentSetXModeHold(std::chrono::nanoseconds hold)
{
    unsigned long long timeVal = hold.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_X_MODE_HOLD);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentSetPeriod(std::chrono::nanoseconds period)
{
    unsigned long long timeVal = period.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_PERIOD);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void memBusAgentSetOutputDelay(std::chrono::nanoseconds delay)
{
    unsigned long long timeVal = delay.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_OUTPUT_DELAY);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriverStart()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_START);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriverStop()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_STOP);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriverFlush()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_FLUSH);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


bool canAgentDriverGetProgress()
{
    char vpiDataOut[VPI_DBUF_SIZE];

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_GET_PROGRESS);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    vpi_end_handshake();

    if (vpiDataOut[0] == '1')
        return true;
    return false;
}


char canAgentDriverGetDrivenVal()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_GET_DRIVEN_VAL);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    vpi_end_handshake();

    return vpiDataOut[0];
}


void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    vpiDataIn.append(1, drivenValue);    // Driven value
    vpiDataIn.append("0");      // No message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_PUSH_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriverPushItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    std::string vpiMsg = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    // TODO: Do we have correct order of characters??
    for (int i = 0; i < strLen; i++)
        vpiMsg.append(std::bitset<8>(int(msg.c_str()[i])).to_string());

    vpiDataIn.append(1, drivenValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_PUSH_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_drive_str_value(VPI_STR_BUF_IN, (char*)vpiMsg.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriverSetWaitTimeout(std::chrono::nanoseconds timeout)
{
    unsigned long long timeVal = timeout.count() * 1000000;
    std::string vpiDataIn = std::bitset<64>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_SET_WAIT_TIMEOUT);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriverWaitFinish()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_WAIT_FINISH);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    std::string vpiMsg = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    // TODO: Do we have correct order of characters??
    for (int i = 0; i < strLen; i++)
        vpiMsg.append(std::bitset<8>(int(msg.c_str()[i])).to_string());

    vpiDataIn.append(1, drivenValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_drive_str_value(VPI_STR_BUF_IN, (char*)vpiMsg.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriveSingleItem(char drivenValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    vpiDataIn.append(1, drivenValue);    // Driven value
    vpiDataIn.append("0");         // No message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_DRIVE_SINGLE_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentDriveAllItems()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_DRIVER_DRIVE_ALL_ITEM);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorStart()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_START);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorStop()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_STOP);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorFlush()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_FLUSH);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


CanAgentMonitorState canAgentMonitorGetState()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    CanAgentMonitorState retVal;
    memset(vpiDataOut, 0, sizeof(vpiDataOut));

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_GET_STATE);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    printf("Read data for monitor state: %s", vpiDataOut);

    if (!strcmp(vpiDataOut, "000"))
        retVal = CAN_AGENT_MONITOR_DISABLED;
    else if (!strcmp(vpiDataOut, "001"))
        retVal = CAN_AGENT_MONITOR_WAITING_FOR_TRIGGER;
    else if (!strcmp(vpiDataOut, "010"))
        retVal = CAN_AGENT_MONITOR_RUNNING;
    else if (!strcmp(vpiDataOut, "011"))
        retVal = CAN_AGENT_MONITOR_PASSED;
    else
        retVal = CAN_AGENT_MONITOR_FAILED;

    vpi_end_handshake();
    return retVal;
}


char canAgentMonitorGetMonitoredVal()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    memset(vpiDataOut, 0, sizeof(vpiDataOut));

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_GET_MONITORED_VAL);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    vpi_end_handshake();

    return vpiDataOut[0];
}


void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    vpiDataIn.append(1, monitorValue);    // Driven value
    vpiDataIn.append("0");      // No message
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_PUSH_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorPushItem(char monitorValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    std::string vpiMsg = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    // TODO: Do we have correct order of characters??
    for (int i = 0; i < strLen; i++)
        vpiMsg.append(std::bitset<8>(int(msg.c_str()[i])).to_string());

    vpiDataIn.append(1, monitorValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_PUSH_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_drive_str_value(VPI_STR_BUF_IN, (char*)vpiMsg.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorSetWaitTimeout(std::chrono::nanoseconds timeout)
{
    unsigned long long timeVal = timeout.count() * 1000000;
    std::string vpiDataIn = std::bitset<64>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_SET_WAIT_TIMEOUT);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorWaitFinish()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_WAIT_FINISH);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";

    vpiDataIn.append(1, monitorValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorSingleItem(char monitorValue, std::chrono::nanoseconds duration, std::string msg)
{
    unsigned long long timeVal = duration.count() * 1000000;
    std::string vpiDataIn = "";
    std::string vpiMsg = "";
    int strLen = msg.length();

    if (strLen > VPI_STR_BUF_SIZE)
        strLen = VPI_STR_BUF_SIZE;

    // TODO: Do we have correct order of characters??
    for (int i = 0; i < strLen; i++)
        vpiMsg.append(std::bitset<8>(int(msg.c_str()[i])).to_string());

    vpiDataIn.append(1, monitorValue); // Driven value
    vpiDataIn.append("1");   // Message included
    vpiDataIn.append(std::bitset<VPI_DBUF_SIZE-2>(timeVal).to_string()); // Drive time

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_MONITOR_SINGLE_ITEM);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_drive_str_value(VPI_STR_BUF_IN, (char*)vpiMsg.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void canAgentMonitorAllItems()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_MONITOR_ALL_ITEMS);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
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

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_GET_STATE);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char *)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


CanAgentMonitorTrigger canAgentMonitorGetTrigger()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    memset(vpiDataOut, 0, sizeof(vpiDataOut));

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_GET_STATE);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    vpi_end_handshake();

    if (!strcmp(vpiDataOut, "000"))
        return CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY;
    if (!strcmp(vpiDataOut, "001"))
        return CAN_AGENT_MONITOR_TRIGGER_RX_RISING;
    if (!strcmp(vpiDataOut, "010"))
        return CAN_AGENT_MONITOR_TRIGGER_RX_FALLING;
    if (!strcmp(vpiDataOut, "011"))
        return CAN_AGENT_MONITOR_TRIGGER_TX_RISING;
    if (!strcmp(vpiDataOut, "100"))
        return CAN_AGENT_MONITOR_TRIGGER_TX_FALLING;
    if (!strcmp(vpiDataOut, "101"))
        return CAN_AGENT_MONITOR_TRIGGER_TIME_ELAPSED;
    if (!strcmp(vpiDataOut, "110"))
        return CAN_AGENT_MONITOR_TRIGGER_DRIVER_START;
    if (!strcmp(vpiDataOut, "111"))
        return CAN_AGENT_MONITOR_TRIGGER_DRIVER_STOP;

    return CAN_AGENT_MONITOR_TRIGGER_IMMEDIATELY;
}


void canAgentMonitorSetSampleRate(std::chrono::nanoseconds sampleRate)
{
    unsigned long long timeVal = sampleRate.count() * 1000000;
    std::string vpiDataIn = std::bitset<64>(timeVal).to_string();

    vpi_drive_str_value(VPI_SIGNAL_DEST, (char*)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_SET_SAMPLE_RATE);
    vpi_drive_str_value(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


std::chrono::nanoseconds canAgentMonitorgetSampleRate()
{
    char vpiDataOut[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpiDataOut, 0, sizeof(vpiDataOut));
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_GET_SAMPLE_RATE);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_begin_handshake();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_read_str_value(VPI_SIGNAL_DATA_OUT, vpiDataOut);
    readTime = std::strtoll(vpiDataOut, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


void canAgentCheckResult()
{
    vpi_drive_str_value(VPI_SIGNAL_DEST, (char *)VPI_DEST_CAN_AGENT);
    vpi_drive_str_value(VPI_SIGNAL_CMD, (char*)VPI_CAN_AGNT_MONITOR_CHECK_RESULT);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vpi_full_handshake();
}


void testControllerAgentEndTest(bool success)
{
    if (success)
        vpi_drive_str_value(VPI_SIGNAL_TEST_RESULT, (char*)"1");
    else
        vpi_drive_str_value(VPI_SIGNAL_TEST_RESULT, (char*)"0");

    vpi_drive_str_value(VPI_SIGNAL_TEST_END, (char*)"1");
}