/**
 * TODO:License
 */

#include <unistd.h>
#include <stdlib.h>
#include <bitset>
#include <string>

extern "C" {
    #include "vpi_utils.h"
}

#include "vpi_compliance_lib.hpp"

/*****************************************************************************
 * Reset agent functions
 ****************************************************************************/

void reset_agent_assert()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_ASSERT);
    vpi_full_handshake();
}


void reset_agent_deassert()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_DEASSERT);
    vpi_full_handshake();
}


void reset_agent_polarity_set(int polarity)
{
    char pol_str[2];
    sprintf(pol_str, "%d", polarity);
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_POLARITY_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, pol_str);
    vpi_full_handshake();
}


int reset_agent_polarity_get()
{
    char pol_str[VPI_DBUF_SIZE];
    memset(pol_str, 0, sizeof(pol_str));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_POLARITY_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, pol_str);
    vpi_end_handshake();
    return atoi(&(pol_str[0]));
}


/*****************************************************************************
 * Clock generator agent functions
 ****************************************************************************/

int clock_agent_start()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_START);
    vpi_full_handshake();
}


int clock_agent_stop()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_STOP);
    vpi_full_handshake();
}


int clock_agent_set_period(std::chrono::nanoseconds clockPeriod)
{
    unsigned long long timeVal = clockPeriod.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char *)binStr.c_str());
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_PERIOD_SET);

    vpi_full_handshake();
}


std::chrono::nanoseconds clock_agent_get_period()
{
    char vpi_data_out[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_PERIOD_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    readTime = std::strtoll(vpi_data_out, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


int clock_agent_set_jitter(std::chrono::nanoseconds clockPeriod)
{
    unsigned long long timeVal = clockPeriod.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_JITTER_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    vpi_full_handshake();
}


std::chrono::nanoseconds clock_agent_get_jitter()
{
    char vpi_data_out[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_JITTER_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    readTime = std::strtoll(vpi_data_out, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


int clock_agent_duty_set(int duty)
{
    std::string binValStr = std::bitset<VPI_DBUF_SIZE>(duty).to_string();
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_DUTY_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)binValStr.c_str());
    vpi_full_handshake();
}


int clock_agent_duty_get()
{
    char vpi_data_out[VPI_DBUF_SIZE];
    int duty;

    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_DUTY_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    duty = std::stoi(vpi_data_out, nullptr, 2);
    vpi_end_handshake();

    return duty;
}


/*****************************************************************************
 * Memory bus agent functions
 ****************************************************************************/

void mem_bus_agent_start()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_START);
    vpi_full_handshake();
}


void mem_bus_agent_stop()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_STOP);
    vpi_full_handshake();
}


void mem_bus_agent_write32(int address, uint32_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("10"); // 32 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append(std::bitset<32>(data).to_string());
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_WRITE);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_full_handshake();
}


void mem_bus_agent_write16(int address, uint16_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("01"); // 16 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("0000000000000000");
    vpiDataIn.append(std::bitset<16>(data).to_string());
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_WRITE);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_full_handshake();
}


void mem_bus_agent_write8(int address, uint8_t data)
{
    std::string vpiDataIn = "1"; // Use blocking write
    vpiDataIn.append("00"); // 8 bit write
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("000000000000000000000000");
    vpiDataIn.append(std::bitset<8>(data).to_string());
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_WRITE);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());
    vpi_full_handshake();
}


uint32_t mem_bus_agent_read32(int address)
{
    std::string vpiDataIn = "";
    char vpi_data_out[VPI_DBUF_SIZE];
    char *endPtr = NULL;
    uint32_t readData;

    vpiDataIn.append("10"); // 32 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_READ);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());

    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    printf("VPI DATA BUFFER READ: %s\n\n", vpi_data_out);

    readData = (uint32_t)strtoul(vpi_data_out, &endPtr, 2);
    vpi_end_handshake();

    return readData;
}

uint16_t mem_bus_agent_read16(int address)
{
    std::string vpiDataIn = "";
    char vpi_data_out[VPI_DBUF_SIZE];
    uint16_t readData;

    vpiDataIn.append("01"); // 16 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_READ);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());

    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    readData = (uint16_t)strtoul(vpi_data_out, NULL, 2);
    vpi_end_handshake();

    return readData;
}


uint8_t mem_bus_agent_read8(int address)
{
    std::string vpiDataIn = "";
    char vpi_data_out[VPI_DBUF_SIZE];
    char vpi_data_8[8];
    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    memset(vpi_data_8, 0, sizeof(vpi_data_8));
    uint8_t readData;

    vpiDataIn.append("00"); // 8 bit access
    vpiDataIn.append(std::bitset<16>(address).to_string());
    vpiDataIn.append("00000000000000000000000000000000");
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_READ);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)vpiDataIn.c_str());

    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    readData = (uint8_t)strtoul(vpi_data_out, NULL, 2);
    vpi_end_handshake();

    return readData;
}


void mem_bus_agent_x_mode_start()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_X_MODE_START);
    vpi_full_handshake();
}


void mem_bus_agent_x_mode_stop()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_X_MODE_STOP);
    vpi_full_handshake();
}


void mem_bus_agent_set_x_mode_setup(std::chrono::nanoseconds setup)
{
    unsigned long long timeVal = setup.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_X_MODE_SETUP);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    vpi_full_handshake();
}


void mem_bus_agent_set_x_mode_hold(std::chrono::nanoseconds hold)
{
    unsigned long long timeVal = hold.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_X_MODE_HOLD);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    vpi_full_handshake();
}


void mem_bus_agent_set_period(std::chrono::nanoseconds period)
{
    unsigned long long timeVal = period.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_PERIOD);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    vpi_full_handshake();
}


void mem_bus_agent_set_output_delay(std::chrono::nanoseconds delay)
{
    unsigned long long timeVal = delay.count() * 1000000;
    std::string binStr = std::bitset<VPI_DBUF_SIZE>(timeVal).to_string();

    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_MEM_BUS_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_MEM_BUS_AGNT_SET_OUTPUT_DELAY);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, (char*)binStr.c_str());
    vpi_full_handshake();
}